/**
 * Copyright (c) 2023, Ouster, Inc.
 * All rights reserved.
 */

#include <gtest/gtest.h>

#include <map>
#include <numeric>
#include <random>
#include <set>

#include "ouster/impl/packet_writer.h"
#include "ouster/impl/profile_extension.h"
#include "ouster/lidar_scan.h"
#include "ouster/pcap.h"
#include "ouster/types.h"
#include "util.h"

using namespace ouster;
using namespace ouster::sensor;
using namespace ouster::sensor::impl;

using test_param = std::tuple<UDPProfileLidar, size_t, size_t, size_t>;
class ScanBatcherTest : public ::testing::TestWithParam<test_param> {};

// clang-format off
INSTANTIATE_TEST_CASE_P(
    ScanBatcherTests,
    ScanBatcherTest,
    ::testing::Combine(
        ::testing::Values(
            UDPProfileLidar::PROFILE_LIDAR_LEGACY,
            UDPProfileLidar::PROFILE_RNG19_RFL8_SIG16_NIR16_DUAL,
            UDPProfileLidar::PROFILE_RNG19_RFL8_SIG16_NIR16,
            UDPProfileLidar::PROFILE_RNG15_RFL8_NIR8,
            UDPProfileLidar::PROFILE_FUSA_RNG15_RFL8_NIR8_DUAL),
        ::testing::Values(1024), // columns_per_frame
        ::testing::Values(128),  // pixels_per_column
        ::testing::Values(16))); // columns_per_packet
// clang-format on

std::vector<LidarPacket> random_frame(UDPProfileLidar profile,
                                      size_t columns_per_frame,
                                      size_t pixels_per_column,
                                      size_t columns_per_packet) {
    packet_writer pw(profile, pixels_per_column, columns_per_packet);

    auto ls = LidarScan(columns_per_frame, pixels_per_column, profile,
                        columns_per_packet);

    ls.frame_id = 700;
    std::iota(ls.measurement_id().data(),
              ls.measurement_id().data() + ls.measurement_id().size(), 0);
    std::iota(ls.packet_timestamp().data(),
              ls.packet_timestamp().data() + ls.packet_timestamp().size(), 10);
    std::iota(ls.timestamp().data(),
              ls.timestamp().data() + ls.timestamp().size(), 1000);
    std::fill(ls.status().data(), ls.status().data() + ls.status().size(), 0x1);

    auto randomise = [&](auto ref_field, const std::string& i) {
        randomize_field(ref_field, pw.field_value_mask(i));
    };
    ouster::impl::foreach_channel_field(ls, pw, randomise);

    auto g = std::mt19937(0xdeadbeef);
    auto dinit_id = std::uniform_int_distribution<uint32_t>(0, 0xFFFFFF);
    auto dserial_no = std::uniform_int_distribution<uint64_t>(0, 0xFFFFFFFFFF);

    uint32_t init_id = dinit_id(g);      // 24 bits
    uint64_t serial_no = dserial_no(g);  // 40 bits

    auto packets = std::vector<LidarPacket>{};
    ouster::impl::scan_to_packets(ls, pw, std::back_inserter(packets), init_id,
                                  serial_no);

    return packets;
}

TEST_P(ScanBatcherTest, scan_batcher_skips_test) {
    auto param = GetParam();
    UDPProfileLidar profile = std::get<0>(param);
    size_t columns_per_frame = std::get<1>(param);
    size_t pixels_per_column = std::get<2>(param);
    size_t columns_per_packet = std::get<3>(param);

    auto packets = random_frame(profile, columns_per_frame, pixels_per_column,
                                columns_per_packet);

    packet_format pf(profile, pixels_per_column, columns_per_packet);
    packet_writer pw(profile, pixels_per_column, columns_per_packet);

    auto reference = LidarScan(columns_per_frame, pixels_per_column, profile,
                               columns_per_packet);
    {
        ScanBatcher batcher(columns_per_frame, pf);
        for (size_t i = 0; i < packets.size(); i++) {
            const auto& p = packets[i];
            if (i == 63) {
                EXPECT_TRUE(batcher(p, reference));
            } else {
                EXPECT_FALSE(batcher(p, reference));
            }
        }
    }

    uint32_t frame_id = pf.frame_id(packets[0].buf.data());

    auto next_frame_packet =
        std::make_unique<LidarPacket>(pf.lidar_packet_size);
    pw.set_frame_id(next_frame_packet->buf.data(), frame_id + 1);

    // produce a reordered packet from "previous frame" with data from one of
    // the dropped packets, therefore we will know if it gets parsed
    auto reordered_packet = std::make_unique<LidarPacket>();
    *reordered_packet = packets.back();
    pw.set_frame_id(reordered_packet->buf.data(), frame_id - 1);

    std::set<uint16_t> invalid_m_ids;
    // dropping in reverse order for easier erase
    std::vector<size_t> dropped_packets = {packets.size() - 1,
                                           packets.size() / 2, 0};
    std::set<uint16_t> dropped_m_ids;
    for (const auto& p_id : dropped_packets) {
        auto& packet = packets[p_id];
        for (size_t icol = 0; icol < columns_per_packet; ++icol) {
            const uint8_t* col_buf = pf.nth_col(icol, packet.buf.data());
            dropped_m_ids.insert(pf.col_measurement_id(col_buf));
        }
        packets.erase(packets.begin() + p_id);
    }
    {  // invalidate every fourth m_id;
        for (auto& packet : packets) {
            for (size_t icol = 0; icol < columns_per_packet; icol += 4) {
                uint8_t* col_buf = pw.nth_col(icol, packet.buf.data());
                invalid_m_ids.insert(pf.col_measurement_id(col_buf));
                pw.set_col_status(col_buf, 0);
            }
        }
    }
    {  // invalidate first half of second packet
        auto& packet = packets[2];
        for (size_t icol = 0; icol < columns_per_packet / 2; ++icol) {
            uint8_t* col_buf = pw.nth_col(icol, packet.buf.data());
            invalid_m_ids.insert(pf.col_measurement_id(col_buf));
            pw.set_col_status(col_buf, 0);
        }
    }
    {  // invalidate last half of fourth packet
        auto& packet = packets[4];
        for (size_t icol = columns_per_packet / 2; icol < columns_per_packet;
             ++icol) {
            uint8_t* col_buf = pw.nth_col(icol, packet.buf.data());
            invalid_m_ids.insert(pf.col_measurement_id(col_buf));
            pw.set_col_status(col_buf, 0);
        }
    }
    {  // invalidate entire tenth packet (packet_timestamp should stay)
        auto& packet = packets[10];
        for (size_t icol = 0; icol < columns_per_packet; ++icol) {
            uint8_t* col_buf = pw.nth_col(icol, packet.buf.data());
            invalid_m_ids.insert(pf.col_measurement_id(col_buf));
            pw.set_col_status(col_buf, 0);
        }
    }
    // swap two packets
    std::swap(packets[4], packets[5]);

    std::set<uint16_t> valid_m_ids;
    for (uint16_t m_id = 0; m_id < columns_per_frame; ++m_id) {
        if (!invalid_m_ids.count(m_id) && !dropped_m_ids.count(m_id))
            valid_m_ids.insert(m_id);
    }

    auto ls = LidarScan(columns_per_frame, pixels_per_column, profile,
                        columns_per_packet);

    {  // pre-fill lidar scan so we know which fields/headers are changed
        auto fill = [](auto ref_field, const std::string&) { ref_field = 1; };
        ouster::impl::foreach_channel_field(ls, pf, fill);
        ls.packet_timestamp() = 2000;
        ls.timestamp() = 100;
        ls.status() = 0x0f;
        ls.measurement_id() = 10000;
    }

    ScanBatcher batcher(columns_per_frame, pf);
    for (const auto& p : packets) {
        EXPECT_FALSE(batcher(p, ls));
    }
    // this should just be dropped, we check that the values are not read in
    // dropped_m_ids checks
    EXPECT_FALSE(batcher(*reordered_packet, ls));
    // cache next_frame_packet, this should finalise writing to ls
    EXPECT_TRUE(batcher(*next_frame_packet, ls));

    EXPECT_EQ(ls.frame_id, frame_id);

    auto test_skipped_fields = [&](auto ref_field, const std::string& chan) {
        // dropped frames should all be zero, RAW_HEADERS or not
        for (auto& m_id : dropped_m_ids) {
            EXPECT_TRUE((ref_field.col(m_id) == 0).all());
        }

        if (chan == ChanField::RAW_HEADERS) {
            for (auto& m_id : valid_m_ids) {
                EXPECT_FALSE((ref_field.col(m_id) == 0).all());
            }
            return;
        }

        using T = typename decltype(ref_field)::Scalar;
        const auto& f = reference.field<T>(chan);
        for (auto& m_id : valid_m_ids) {
            EXPECT_TRUE((ref_field.col(m_id) == f.col(m_id)).all());
        }

        // these should be zero unless RAW_HEADERS
        for (auto& m_id : invalid_m_ids) {
            EXPECT_TRUE((ref_field.col(m_id) == 0).all());
        }
    };

    auto test_headers = [&](const LidarScan& scan) {
        for (auto& m_id : dropped_m_ids) {
            EXPECT_EQ(scan.timestamp()[m_id], 0);
            EXPECT_EQ(scan.status()[m_id], 0);
            EXPECT_EQ(scan.measurement_id()[m_id], 0);
        }

        for (auto& m_id : invalid_m_ids) {
            EXPECT_EQ(scan.timestamp()[m_id], 0);
            EXPECT_EQ(scan.status()[m_id], 0);
            EXPECT_EQ(scan.measurement_id()[m_id], 0);
        }

        for (auto& m_id : valid_m_ids) {
            EXPECT_NE(scan.timestamp()[m_id], 0);
            EXPECT_NE(scan.status()[m_id], 0);
            EXPECT_EQ(scan.measurement_id()[m_id], m_id);
        }

        for (const auto& p_id : dropped_packets)
            EXPECT_EQ(scan.packet_timestamp()[p_id], 0);
        EXPECT_EQ((scan.packet_timestamp() == 0).count(),
                  dropped_packets.size());
    };

    ouster::impl::foreach_channel_field(ls, pf, test_skipped_fields);
    test_headers(ls);

    // now repeat for RAW_HEADERS and CUSTOM fields
    LidarScanFieldTypes rh_types(ls.field_types());
    rh_types.emplace_back(ChanField::RAW_HEADERS, ChanFieldType::UINT32);
    rh_types.emplace_back("CUSTOM0", ChanFieldType::UINT32);
    rh_types.emplace_back("CUSTOM9", ChanFieldType::UINT32);
    auto rh_ls =
        LidarScan(columns_per_frame, pixels_per_column, rh_types.begin(),
                  rh_types.end(), columns_per_packet);

    {  // pre-fill lidar scan so we know which fields/headers are changed
        auto fill = [](auto ref_field, const std::string&) { ref_field = 1; };
        ouster::impl::foreach_channel_field(rh_ls, pf, fill);
        ouster::impl::visit_field(rh_ls, "CUSTOM0", fill, "");
        ouster::impl::visit_field(rh_ls, "CUSTOM9", fill, "");
        rh_ls.packet_timestamp() = 2000;
        rh_ls.timestamp() = 100;
        rh_ls.status() = 0x0f;
        rh_ls.measurement_id() = 10000;
    }

    ScanBatcher rh_batcher(columns_per_frame, pf);
    for (const auto& p : packets) {
        EXPECT_FALSE(rh_batcher(p, rh_ls));
    }
    EXPECT_FALSE(rh_batcher(*reordered_packet, rh_ls));
    EXPECT_TRUE(rh_batcher(*next_frame_packet, rh_ls));

    EXPECT_EQ(rh_ls.frame_id, frame_id);

    auto test_custom_fields = [](auto ref_field) {
        EXPECT_TRUE((ref_field == 1).all());
    };

    ouster::impl::visit_field(rh_ls, "CUSTOM0", test_custom_fields);
    ouster::impl::visit_field(rh_ls, "CUSTOM9", test_custom_fields);

    ouster::impl::foreach_channel_field(rh_ls, pf, test_skipped_fields);
    ouster::impl::visit_field(rh_ls, ChanField::RAW_HEADERS,
                              test_skipped_fields, ChanField::RAW_HEADERS);
    test_headers(rh_ls);
}

/**
 * repeat the above test for block traversal case (no invalid m_ids in packets)
 */
TEST_P(ScanBatcherTest, scan_batcher_block_parse_dropped_packets_test) {
    auto param = GetParam();
    UDPProfileLidar profile = std::get<0>(param);
    size_t columns_per_frame = std::get<1>(param);
    size_t pixels_per_column = std::get<2>(param);
    size_t columns_per_packet = std::get<3>(param);

    auto packets = random_frame(profile, columns_per_frame, pixels_per_column,
                                columns_per_packet);

    packet_format pf(profile, pixels_per_column, columns_per_packet);
    packet_writer pw(profile, pixels_per_column, columns_per_packet);

    auto reference = LidarScan(columns_per_frame, pixels_per_column, profile,
                               columns_per_packet);
    {
        ScanBatcher batcher(columns_per_frame, pf);
        for (size_t i = 0; i < packets.size(); i++) {
            const auto& p = packets[i];
            if (i == 63) {
                EXPECT_TRUE(batcher(p, reference));
            } else {
                EXPECT_FALSE(batcher(p, reference));
            }
        }
    }

    uint32_t frame_id = pf.frame_id(packets[0].buf.data());

    auto next_frame_packet =
        std::make_unique<LidarPacket>(pf.lidar_packet_size);
    pw.set_frame_id(next_frame_packet->buf.data(), frame_id + 1);

    // dropping in reverse order for easier erase
    std::vector<size_t> dropped_packets = {packets.size() - 1,
                                           packets.size() / 2, 0};
    std::set<uint16_t> dropped_m_ids;
    for (const auto& p_id : dropped_packets) {
        auto& packet = packets[p_id];
        for (size_t icol = 0; icol < columns_per_packet; ++icol) {
            const uint8_t* col_buf = pf.nth_col(icol, packet.buf.data());
            dropped_m_ids.insert(pf.col_measurement_id(col_buf));
        }
        packets.erase(packets.begin() + p_id);
    }
    // swap two packets
    std::swap(packets[4], packets[5]);

    std::set<uint16_t> valid_m_ids;
    for (uint16_t m_id = 0; m_id < columns_per_frame; ++m_id) {
        if (!dropped_m_ids.count(m_id)) valid_m_ids.insert(m_id);
    }

    auto ls = LidarScan(columns_per_frame, pixels_per_column, profile,
                        columns_per_packet);

    {  // pre-fill lidar scan so we know which fields/headers are changed
        auto fill = [](auto ref_field, const std::string&) { ref_field = 1; };
        ouster::impl::foreach_channel_field(ls, pf, fill);
        ls.packet_timestamp() = 2000;
        ls.timestamp() = 100;
        ls.status() = 0x0f;
        ls.measurement_id() = 10000;
    }

    ScanBatcher batcher(columns_per_frame, pf);
    for (const auto& p : packets) {
        EXPECT_FALSE(batcher(p, ls));
    }
    EXPECT_TRUE(batcher(*next_frame_packet, ls));

    auto test_skipped_fields = [&](auto ref_field, const std::string& chan) {
        // dropped frames should all be zero, RAW_HEADERS or not
        for (auto& m_id : dropped_m_ids) {
            EXPECT_TRUE((ref_field.col(m_id) == 0).all());
        }

        using T = typename decltype(ref_field)::Scalar;
        const auto& f = reference.field<T>(chan);
        for (auto& m_id : valid_m_ids) {
            EXPECT_TRUE((ref_field.col(m_id) == f.col(m_id)).all());
        }
    };

    auto test_headers = [&](const LidarScan& scan) {
        for (auto& m_id : dropped_m_ids) {
            EXPECT_EQ(scan.timestamp()[m_id], 0);
            EXPECT_EQ(scan.status()[m_id], 0);
            EXPECT_EQ(scan.measurement_id()[m_id], 0);
        }

        for (auto& m_id : valid_m_ids) {
            EXPECT_NE(scan.timestamp()[m_id], 0);
            EXPECT_NE(scan.status()[m_id], 0);
            EXPECT_EQ(scan.measurement_id()[m_id], m_id);
        }

        for (const auto& p_id : dropped_packets)
            EXPECT_EQ(scan.packet_timestamp()[p_id], 0);
        EXPECT_EQ((scan.packet_timestamp() == 0).count(),
                  dropped_packets.size());
    };

    ouster::impl::foreach_channel_field(ls, pf, test_skipped_fields);
    test_headers(ls);

    // now repeat for CUSTOM fields
    LidarScanFieldTypes custom_types(ls.field_types());
    custom_types.emplace_back("CUSTOM0", ChanFieldType::UINT32);
    custom_types.emplace_back("CUSTOM9", ChanFieldType::UINT32);
    auto custom_ls =
        LidarScan(columns_per_frame, pixels_per_column, custom_types.begin(),
                  custom_types.end(), columns_per_packet);

    {  // pre-fill lidar scan so we know which fields/headers are changed
        auto fill = [](auto ref_field, const std::string&) { ref_field = 1; };
        ouster::impl::foreach_channel_field(custom_ls, pf, fill);
        ouster::impl::visit_field(custom_ls, "CUSTOM0", fill, "");
        ouster::impl::visit_field(custom_ls, "CUSTOM9", fill, "");
        custom_ls.packet_timestamp() = 2000;
        custom_ls.timestamp() = 100;
        custom_ls.status() = 0x0f;
        custom_ls.measurement_id() = 10000;
    }

    ScanBatcher custom_batcher(columns_per_frame, pf);
    for (const auto& p : packets) {
        EXPECT_FALSE(custom_batcher(p, custom_ls));
    }
    EXPECT_TRUE(custom_batcher(*next_frame_packet, custom_ls));

    EXPECT_EQ(custom_ls.frame_id, frame_id);

    auto test_custom_fields = [](auto ref_field) {
        EXPECT_TRUE((ref_field == 1).all());
    };

    ouster::impl::visit_field(custom_ls, "CUSTOM0", test_custom_fields);
    ouster::impl::visit_field(custom_ls, "CUSTOM9", test_custom_fields);

    ouster::impl::foreach_channel_field(custom_ls, pf, test_skipped_fields);
    test_headers(custom_ls);
}

TEST_P(ScanBatcherTest, scan_batcher_wraparound_test) {
    auto param = GetParam();
    UDPProfileLidar profile = std::get<0>(param);
    size_t columns_per_frame = std::get<1>(param);
    size_t pixels_per_column = std::get<2>(param);
    size_t columns_per_packet = std::get<3>(param);

    auto ls = LidarScan(columns_per_frame, pixels_per_column, profile,
                        columns_per_packet);

    packet_format pf(profile, pixels_per_column, columns_per_packet);
    packet_writer pw(profile, pixels_per_column, columns_per_packet);

    // pre-fill lidar scan so we know which fields/headers are changed
    auto fill = [](auto ref_field, const std::string&) { ref_field = 1; };
    ouster::impl::foreach_channel_field(ls, pf, fill);
    ls.packet_timestamp() = 2000;
    ls.timestamp() = 100;
    ls.status() = 0x0f;
    ls.measurement_id() = 10000;
    ls.frame_id = 0;

    auto packet = std::make_unique<LidarPacket>(pf.lidar_packet_size);
    std::memset(packet->buf.data(), 0, packet->buf.size());
    packet->host_timestamp = 100;
    pw.set_frame_id(packet->buf.data(), pw.max_frame_id);
    uint16_t m_id = columns_per_frame - columns_per_packet;
    uint64_t ts = 100;
    for (size_t icol = 0; icol < columns_per_packet; ++icol) {
        uint8_t* col_buf = pw.nth_col(icol, packet->buf.data());
        pw.set_col_status(col_buf, 0x01 /*valid*/);
        pw.set_col_measurement_id(col_buf, m_id++);
        pw.set_col_timestamp(col_buf, ts++);
    }

    ScanBatcher batcher(columns_per_frame, pf);
    EXPECT_FALSE(batcher(*packet, ls));

    EXPECT_EQ(ls.frame_id, 0);
    EXPECT_TRUE((ls.packet_timestamp() == 2000).all());
    EXPECT_TRUE((ls.timestamp() == 100).all());
    EXPECT_TRUE((ls.status() == 0x0f).all());
    EXPECT_TRUE((ls.measurement_id() == 10000).all());
    auto test_fields = [](auto ref_field, const std::string&) {
        EXPECT_TRUE((ref_field == 1).all());
    };
    ouster::impl::foreach_channel_field(ls, pf, test_fields);
}

using HashMap = std::map<std::string, size_t>;
using snapshot_param = std::tuple<std::string, std::string, HashMap>;
class ScanBatcherSnapshotTest
    : public ::testing::TestWithParam<snapshot_param> {};

// clang-format off
INSTANTIATE_TEST_CASE_P(
    ScanBatcherSnapshots,
    ScanBatcherSnapshotTest,
    ::testing::Values(
        // low bandwidth
        snapshot_param{"OS-0-128-U1_v2.3.0_1024x10.pcap",
                       "OS-0-128-U1_v2.3.0_1024x10.json",
                       {{ChanField::RANGE, 0xf605c68634d4d496},
                        {ChanField::REFLECTIVITY, 0x308446ce12113b5c},
                        {ChanField::NEAR_IR, 0xacbe4e6963b1d6c7},
                        {ChanField::FLAGS, 6373750807750774351}}},
        // dual return
        snapshot_param{"OS-0-32-U1_v2.2.0_1024x10.pcap",
                       "OS-0-32-U1_v2.2.0_1024x10.json",
                       {{ChanField::RANGE, 0xda815ba0ea0173dd},
                        {ChanField::RANGE2, 0x9d07c3e610c99239},
                        {ChanField::SIGNAL, 0xb2d846ac47621f7b},
                        {ChanField::SIGNAL2, 0x4553138a62c59e37},
                        {ChanField::REFLECTIVITY, 0x63d4c6e69ced4423},
                        {ChanField::REFLECTIVITY2, 0x415f5e481688fe5a},
                        {ChanField::NEAR_IR, 0x2c32a3e5be6b01d5},
                        {ChanField::FLAGS, 6902511898004997142},
                        {ChanField::FLAGS2, 14986456617710294519U}}},
        // fusa dual return
        snapshot_param{"OS-1-128_767798045_1024x10_20230712_120049.pcap",
                       "OS-1-128_767798045_1024x10_20230712_120049.json",
                       {{ChanField::RANGE, 0x8327b9d4c44c45a3},
                        {ChanField::RANGE2, 0x87288b444ddb9c9e},
                        {ChanField::REFLECTIVITY, 0x6912ca3fa04b0d1f},
                        {ChanField::REFLECTIVITY2, 0xf58aa5594d9749dc},
                        {ChanField::NEAR_IR, 0xc99384623c5d9feb},
                        {ChanField::FLAGS, 15585490641324286966U},
                        {ChanField::FLAGS2, 3655442015794344596}}},
        // single return
        snapshot_param{"OS-2-128-U1_v2.3.0_1024x10.pcap",
                       "OS-2-128-U1_v2.3.0_1024x10.json",
                       {{ChanField::RANGE, 0x5940899c1190d02d},
                        {ChanField::SIGNAL, 0x4446bddd21f14dd4},
                        {ChanField::REFLECTIVITY, 0xea599b8814d2eac1},
                        {ChanField::NEAR_IR, 0x8a5a3df8896e317a},
                        {ChanField::FLAGS, 3655442015794344596}}},
        // legacy
        snapshot_param{"OS-2-32-U0_v2.0.0_1024x10.pcap",
                       "OS-2-32-U0_v2.0.0_1024x10.json",
                       {{ChanField::RANGE, 0x5937f3d8f3762184},
                        {ChanField::SIGNAL, 0xbb4b7f22d1231e80},
                        {ChanField::REFLECTIVITY, 0x3D37AAEB2792F714},
                        {ChanField::NEAR_IR, 0xe972940ca8b204f0},
                        {ChanField::FLAGS, 13284364481018348283U}}}));
// clang-format on

// picked up from
// https://wjngkoh.wordpress.com/2015/03/04/c-hash-function-for-eigen-matrix-and-vector/
struct matrix_hash {
    template <typename T>
    void operator()(Eigen::Ref<ouster::img_t<T>> matrix, const std::string& f,
                    HashMap& map) const {
        size_t seed = 0;
        for (int i = 0; i < matrix.size(); ++i) {
            auto elem = *(matrix.data() + i);
            seed ^=
                std::hash<T>()(elem) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        map[f] = seed;
    }
};

using namespace ouster::sensor_utils;

TEST_P(ScanBatcherSnapshotTest, snapshot_test) {
#ifdef _WIN32
    // technically speaking, std::hash is implementation dependent,
    // so a better solution would be to have a cross-platform hash
    GTEST_SKIP();
#endif

    auto data_dir = getenvs("DATA_DIR");
    const auto test_params = GetParam();

    auto info = metadata_from_json(data_dir + "/" + std::get<1>(test_params));
    auto pf = packet_format(info);
    PcapReader pcap(data_dir + "/" + std::get<0>(test_params));

    const HashMap snapshot_hashes = std::get<2>(test_params);

    auto ls = LidarScan(info.format.columns_per_frame, pf.pixels_per_column,
                        pf.udp_profile_lidar, pf.columns_per_packet);
    uint64_t packet_ts = 1234;  // arbitrary, irrelevant for test
    ScanBatcher batcher(ls.w, pf);
    int packet_index = 0;
    while (pcap.next_packet())
        if (pcap.current_info().dst_port == 7502) {
            if (packet_index++ == 63) {
                EXPECT_TRUE(batcher(pcap.current_data(), packet_ts, ls));
            } else {
                EXPECT_FALSE(batcher(pcap.current_data(), packet_ts, ls));
            }
        }

    HashMap hashes;
    ouster::impl::foreach_channel_field(ls, pf, matrix_hash{}, hashes);

    EXPECT_EQ(hashes, snapshot_hashes);
}

namespace alternatives {
using Fields = std::vector<std::pair<std::string, FieldInfo>>;

static const Fields lb_field_info{
    {ChanField::RANGE, {UINT32, 0, 0x7fff, -3}},  // uint16 => uint32
    {ChanField::FLAGS, {UINT8, 1, 0b10000000, 7}},
    {ChanField::REFLECTIVITY, {UINT8, 1, 0xff00, 8}},
    {ChanField::NEAR_IR, {UINT16, 2, 0xff00, 4}}  // uint8  => uint16
};

static const Fields single_field_info{
    {ChanField::RANGE, {UINT32, 0, 0x0007ffff, 0}},
    {ChanField::FLAGS, {UINT8, 2, 0b11111000, 3}},
    {ChanField::REFLECTIVITY, {UINT8, 3, 0xff00, 8}},
    {ChanField::SIGNAL, {UINT16, 6, 0, 0}},
    {ChanField::NEAR_IR, {UINT16, 8, 0, 0}}};

static const Fields fusa_info{
    {ChanField::RANGE, {UINT32, 0, 0x7fff, -3}},  // uint16 => uint32
    {ChanField::REFLECTIVITY, {UINT8, 2, 0xff, 0}},
    {ChanField::NEAR_IR, {UINT16, 3, 0xff, -4}},
    {ChanField::RANGE2, {UINT32, 4, 0x7fff, -3}},  // uint16 => uint32
    {ChanField::REFLECTIVITY2, {UINT8, 6, 0xff, 0}},
    {ChanField::FLAGS, {UINT8, 1, 0b10000000, 7}},
    {ChanField::FLAGS2, {UINT8, 5, 0b10000000, 7}},
};

int add_profiles() {
    add_custom_profile(10 + UDPProfileLidar::PROFILE_RNG15_RFL8_NIR8,
                       "PROFILE_LOWBAND_ALT", lb_field_info, 4);
    add_custom_profile(10 + UDPProfileLidar::PROFILE_RNG19_RFL8_SIG16_NIR16,
                       "PROFILE_SINGLE_ALT", single_field_info, 12);
    add_custom_profile(10 + UDPProfileLidar::PROFILE_FUSA_RNG15_RFL8_NIR8_DUAL,
                       "PROFILE_FUSA_ALT", fusa_info, 8);
    return 1;
}

}  // namespace alternatives

TEST_P(ScanBatcherSnapshotTest, extended_profile_comp_test) {
    static const int profiles_added = alternatives::add_profiles();
    (void)profiles_added;  // suppress stupid warning

    auto data_dir = getenvs("DATA_DIR");
    const auto test_params = GetParam();

    auto info = metadata_from_json(data_dir + "/" + std::get<1>(test_params));
    PcapReader pcap(data_dir + "/" + std::get<0>(test_params));

    UDPProfileLidar prof = info.format.udp_profile_lidar;

    // skip legacy because some parsing specifics are very different
    if (prof == UDPProfileLidar::PROFILE_LIDAR_LEGACY) GTEST_SKIP();
    // skip dual returns because it doesn't have type inconsistencies
    if (prof == UDPProfileLidar::PROFILE_RNG19_RFL8_SIG16_NIR16_DUAL)
        GTEST_SKIP();

    auto get_hashes = [&](UDPProfileLidar profile) -> HashMap {
        HashMap hashes;

        packet_format pf(profile, info.format.pixels_per_column,
                         info.format.columns_per_packet);

        auto ls = LidarScan(info.format.columns_per_frame, pf.pixels_per_column,
                            pf.udp_profile_lidar, pf.columns_per_packet);
        uint64_t packet_ts = 1234;  // arbitrary, irrelevant for test
        ScanBatcher batcher(ls.w, pf);

        int packet_index = 0;
        while (pcap.next_packet()) {
            if (pcap.current_info().dst_port == 7502) {
                if (packet_index++ == 63) {
                    EXPECT_TRUE(batcher(pcap.current_data(), packet_ts, ls));
                } else {
                    EXPECT_FALSE(batcher(pcap.current_data(), packet_ts, ls));
                }
            }
        }
        pcap.seek(0);
        ouster::impl::foreach_channel_field(ls, pf, matrix_hash{}, hashes);
        return hashes;
    };

    UDPProfileLidar p_alt = static_cast<UDPProfileLidar>(10 + prof);

    HashMap hashes_orig = get_hashes(prof);
    HashMap hashes_alt = get_hashes(p_alt);

    EXPECT_EQ(hashes_orig, hashes_alt);
}

TEST(ScanBatcherLegacyTest, legacy_col_status) {
    std::string pcap_file = "OS-2-32-U0_v2.0.0_1024x10.pcap";
    std::string meta_file = "OS-2-32-U0_v2.0.0_1024x10.json";

    auto data_dir = getenvs("DATA_DIR");

    auto info = metadata_from_json(data_dir + "/" + meta_file);
    PcapReader pcap(data_dir + "/" + pcap_file);

    packet_format pf(info);

    while (pcap.next_packet()) {
        if (pcap.current_info().dst_port == 7502) {
            for (int icol = 0; icol < pf.columns_per_packet; icol++) {
                const uint8_t* col_buf = pf.nth_col(icol, pcap.current_data());
                const uint32_t status = pf.col_status(col_buf);
                EXPECT_EQ(status, 0xFFFFFFFF);
            }
        }
    }
}

TEST(ScanBatcherTest, cached_packet_test) {
    data_format df{128,
                   16,
                   1024,
                   {},
                   {0, 1023},
                   PROFILE_RNG15_RFL8_NIR8,
                   PROFILE_IMU_LEGACY,
                   10};
    sensor_info meta{};
    meta.format = df;
    packet_writer pw{meta};

    uint16_t frame_id = 1337;

    auto packets_first =
        random_frame(df.udp_profile_lidar, df.columns_per_frame,
                     df.pixels_per_column, df.columns_per_packet);
    // drop one last packet so that the scan does not finalize
    packets_first.pop_back();

    auto packets_second =
        random_frame(df.udp_profile_lidar, df.columns_per_frame,
                     df.pixels_per_column, df.columns_per_packet);

    for (auto& p : packets_first) {
        pw.set_frame_id(p.buf.data(), frame_id);
    }
    for (auto& p : packets_second) {
        pw.set_frame_id(p.buf.data(), frame_id + 1);
    }

    auto ref_second = LidarScan(df.columns_per_frame, df.pixels_per_column,
                                df.udp_profile_lidar, df.columns_per_packet);
    {  // parse ref_second only for final reference
        ScanBatcher batcher{df.columns_per_frame, pw};
        for (const auto& p : packets_second) {
            batcher(p, ref_second);
        }
    }

    auto ls = LidarScan(df.columns_per_frame, df.pixels_per_column,
                        df.udp_profile_lidar, df.columns_per_packet);
    ScanBatcher batcher{df.columns_per_frame, pw};
    for (const auto& p : packets_first) {
        ASSERT_FALSE(batcher(p, ls));
    }

    LidarScan ref_first = ls;

    // get packet to cache and check lidar scan did not change
    EXPECT_TRUE(batcher(packets_second[0], ls));
    EXPECT_EQ(ls, ref_first);

    std::for_each(packets_second.begin() + 1, packets_second.end() - 1,
                  [&batcher, &ls](const auto& packet) {
                      ASSERT_FALSE(batcher(packet, ls));
                  });
    EXPECT_TRUE(batcher(packets_second.back(), ls));

    // check scan gets fully batched
    EXPECT_EQ(ls, ref_second);
}
