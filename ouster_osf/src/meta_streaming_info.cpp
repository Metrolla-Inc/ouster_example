/**
 * Copyright(c) 2021, Ouster, Inc.
 * All rights reserved.
 */

#include "ouster/osf/meta_streaming_info.h"

#include <jsoncons/json.hpp>
#include <map>
#include <sstream>

#include "ouster/osf/meta_streaming_info.h"
#include "streaming/streaming_info_generated.h"

namespace ouster {
namespace osf {

std::string to_string(const ChunkInfo& chunk_info) {
    std::stringstream ss;
    ss << "{offset = " << chunk_info.offset
       << ", stream_id = " << chunk_info.stream_id
       << ", message_count = " << chunk_info.message_count << "}";
    return ss.str();
}

StreamStats::StreamStats(uint32_t s_id, ts_t receive_ts, ts_t sensor_ts,
                         uint32_t msg_size)
    : stream_id{s_id},
      start_ts{receive_ts},
      end_ts{receive_ts},
      message_count{1},
      message_avg_size{msg_size} {
    receive_timestamps.push_back(receive_ts.count());
    sensor_timestamps.push_back(sensor_ts.count());
}

void StreamStats::update(ts_t receive_ts, ts_t sensor_ts, uint32_t msg_size) {
    if (start_ts > receive_ts) start_ts = receive_ts;
    if (end_ts < receive_ts) end_ts = receive_ts;
    ++message_count;
    int avg_size = static_cast<int>(message_avg_size);
    avg_size = avg_size + (static_cast<int>(msg_size) - avg_size) /
                              static_cast<int>(message_count);
    message_avg_size = static_cast<uint32_t>(avg_size);
    receive_timestamps.push_back(receive_ts.count());
    sensor_timestamps.push_back(sensor_ts.count());
}

std::string to_string(const StreamStats& stream_stats) {
    std::stringstream ss;
    ss << "{stream_id = " << stream_stats.stream_id
       << ", start_ts = " << stream_stats.start_ts.count()
       << ", end_ts = " << stream_stats.end_ts.count()
       << ", message_count = " << stream_stats.message_count
       << ", message_avg_size = " << stream_stats.message_avg_size
       << ", host_timestamps = [";
    for (const auto& ts : stream_stats.receive_timestamps) {
        ss << ts << ", ";
    }
    ss << "], sensor_timestamps = [";
    for (const auto& ts : stream_stats.sensor_timestamps) {
        ss << ts << ", ";
    }
    ss << "]}";
    return ss.str();
}

flatbuffers::Offset<ouster::osf::gen::StreamingInfo> create_streaming_info(
    flatbuffers::FlatBufferBuilder& fbb,
    const std::map<uint64_t, ChunkInfo>& chunks_info,
    const std::map<uint32_t, StreamStats>& stream_stats) {
    // Pack chunks vector
    std::vector<flatbuffers::Offset<gen::ChunkInfo>> chunks_info_vec;
    for (const auto& chunk_info : chunks_info) {
        const auto& ci = chunk_info.second;
        auto ci_offset = gen::CreateChunkInfo(fbb, ci.offset, ci.stream_id,
                                              ci.message_count);
        chunks_info_vec.push_back(ci_offset);
    }

    // Pack stream_stats vector
    std::vector<flatbuffers::Offset<gen::StreamStats>> stream_stats_vec;
    for (const auto& stream_stat : stream_stats) {
        auto stat = stream_stat.second;
        auto ss_offset = gen::CreateStreamStats(
            fbb, stat.stream_id, stat.start_ts.count(), stat.end_ts.count(),
            stat.message_count, stat.message_avg_size,
            fbb.CreateVector<uint64_t>(stat.receive_timestamps),
            fbb.CreateVector<uint64_t>(stat.sensor_timestamps));
        stream_stats_vec.push_back(ss_offset);
    }

    auto si_offset = ouster::osf::gen::CreateStreamingInfoDirect(
        fbb, &chunks_info_vec, &stream_stats_vec);
    return si_offset;
}

StreamingInfo::StreamingInfo(
    const std::vector<std::pair<uint64_t, ChunkInfo>>& chunks_info,
    const std::vector<std::pair<uint32_t, StreamStats>>& stream_stats)
    : chunks_info_{chunks_info.begin(), chunks_info.end()},
      stream_stats_{stream_stats.begin(), stream_stats.end()} {}

StreamingInfo::StreamingInfo(
    const std::map<uint64_t, ChunkInfo>& chunks_info,
    const std::map<uint32_t, StreamStats>& stream_stats)
    : chunks_info_(chunks_info), stream_stats_(stream_stats) {}

std::map<uint64_t, ChunkInfo>& StreamingInfo::chunks_info() {
    return chunks_info_;
}

std::map<uint32_t, StreamStats>& StreamingInfo::stream_stats() {
    return stream_stats_;
}

std::vector<uint8_t> StreamingInfo::buffer() const {
    flatbuffers::FlatBufferBuilder fbb = flatbuffers::FlatBufferBuilder(32768);
    auto si_offset = create_streaming_info(fbb, chunks_info_, stream_stats_);
    fbb.FinishSizePrefixed(si_offset);
    const uint8_t* buf = fbb.GetBufferPointer();
    const size_t size = fbb.GetSize();
    return {buf, buf + size};
};

std::unique_ptr<MetadataEntry> StreamingInfo::from_buffer(
    const std::vector<uint8_t>& buf) {
    auto streaming_info = gen::GetSizePrefixedStreamingInfo(buf.data());

    std::unique_ptr<StreamingInfo> si = std::make_unique<StreamingInfo>();

    auto& chunks_info = si->chunks_info();
    if (streaming_info->chunks() && streaming_info->chunks()->size()) {
        std::transform(
            streaming_info->chunks()->begin(), streaming_info->chunks()->end(),
            std::inserter(chunks_info, chunks_info.end()),
            [](const gen::ChunkInfo* ci) {
                return std::make_pair(ci->offset(),
                                      ChunkInfo{ci->offset(), ci->stream_id(),
                                                ci->message_count()});
            });
    }

    auto& stream_stats = si->stream_stats();
    if (streaming_info->stream_stats() &&
        streaming_info->stream_stats()->size()) {
        std::transform(streaming_info->stream_stats()->begin(),
                       streaming_info->stream_stats()->end(),
                       std::inserter(stream_stats, stream_stats.end()),
                       [](const gen::StreamStats* stat) {
                           StreamStats ss{};
                           ss.stream_id = stat->stream_id();
                           ss.start_ts = ts_t{stat->start_ts()};
                           ss.end_ts = ts_t{stat->end_ts()};
                           ss.message_count = stat->message_count();
                           ss.message_avg_size = stat->message_avg_size();
                           if (stat->receive_timestamps()) {
                               for (auto v : *stat->receive_timestamps()) {
                                   ss.receive_timestamps.push_back(v);
                               }
                           }
                           if (stat->sensor_timestamps()) {
                               for (auto v : *stat->sensor_timestamps()) {
                                   ss.sensor_timestamps.push_back(v);
                               }
                           }
                           return std::make_pair(stat->stream_id(), ss);
                       });
    }

    return si;
}

std::string StreamingInfo::repr() const {
    jsoncons::json si_obj;

    jsoncons::json chunks(jsoncons::json_array_arg);
    for (const auto& ci : chunks_info_) {
        jsoncons::json chunk_info;
        chunk_info["offset"] = static_cast<uint64_t>(ci.second.offset);
        chunk_info["stream_id"] = ci.second.stream_id;
        chunk_info["message_count"] = ci.second.message_count;
        chunks.emplace_back(chunk_info);
    }
    si_obj["chunks"] = chunks;

    jsoncons::json stream_stats(jsoncons::json_array_arg);
    for (const auto& stat : stream_stats_) {
        jsoncons::json ss{};
        ss["stream_id"] = stat.first;
        ss["start_ts"] = static_cast<uint64_t>(stat.second.start_ts.count());
        ss["end_ts"] = static_cast<uint64_t>(stat.second.end_ts.count());
        ss["message_count"] = static_cast<uint64_t>(stat.second.message_count);
        ss["message_avg_size"] = stat.second.message_avg_size;
        jsoncons::json st(jsoncons::json_array_arg);
        jsoncons::json rt(jsoncons::json_array_arg);
        for (const auto& t : stat.second.sensor_timestamps) {
            st.emplace_back(static_cast<uint64_t>(t));
        }
        for (const auto& t : stat.second.receive_timestamps) {
            rt.emplace_back(static_cast<uint64_t>(t));
        }
        ss["sensor_timestamps"] = st;
        ss["receive_timestamps"] = rt;
        stream_stats.emplace_back(ss);
    }
    si_obj["stream_stats"] = stream_stats;

    std::string out;
    si_obj.dump(out);
    return out;
};

}  // namespace osf
}  // namespace ouster
