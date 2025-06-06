=========
Changelog
=========

Important announcements
-----------------------
* As of 0.13.0, the SDK is no longer compatible with firmware versions older than 2.1.0.
* Official SDK support for firmware versions 2.2 and 2.3 will end at the end of June, 2025.
* Update vcpkg ref of build to 2024.11.16

[20250117] [0.14.0]
======================

ouster_client/C++ SDK
---------------------

* Jsoncpp fully removed for jsoncons
* [BREAKING] All the HTTP endpoint methods in the ``SensorHttpImp`` class now return a ``std::string`` instead of a ``Json::Value`` object. The result can be parsed with any json parser.
* Add CMake logic for packaging c++ sdk in binary format when ``-DBUILD_SHARED_LIBRARY=ON`` is enabled. 

ouster_client/Python SDK
------------------------
* Add a new command ``localize`` to perform localization and tracking within a SLAM-generated map of a given site.
* Add ``LidarScan.sensor_info`` to store the relevant ``SensorInfo`` for each scan
* [BREAKING] Deprecated ``ScanBatcher::operator()(const uint8_t*, uint64_t, LidarScan&)`` for ``ScanBatcher::operator()(const LidarPacket&, LidarScan&)``
* [BREAKING] Disabled ``OUSTER_USE_EIGEN_MAX_ALIGN_BYTES_32`` by default to help avoid ABI mismatches
* [BREAKING] Changed ``SensorClient`` ``get_packet`` API to return packet in the ``ClientEvent`` rather than through reference parameters
* Updated to Kiss ICP 1.1.0 version
* [BUGFIX] Fixed OSF failing to load scans saved in 4096 * 5 mode
* [BUGFIX] Fixed Python ``client.transform`` and ``client.dewarp`` methods returning incorrect results due to ignoring column layout of input data
* Refactored logging to remove spdlog API exposure
* Vendored spdlog in third party dependencies
* [BREAKING] Change ``sensor_info.sn`` type from string to uint64_t
* Support additional array types and formats for Cloud.set_xyz
* Added new ``mask``, ``reduce`` and ``clip`` ScanSource operations to the SDK CLI and API
* The ``clip`` command can now specify which fields to be applied to and accepts unit
* Add relevant methods from ``packet_format`` to ``LidarPacket`` and ``ImuPacket`` classes
* Add ``format`` to each ``Packet`` object with the relevant packet format
* Tolerate off-by-1-byte for bag files recorded using an older version of the ouster-ros driver
* Fix yaw axis to zero in the get_rot_matrix_to_align_to_gravity function
* [BUGFIX] Fix the ``-c`` option in ouster-cli ``config`` command to set config from file

ouster_cli
----------
* Add ``ouster-cli source SENSOR set_static_ip`` command to set sensor static IPs.
* Add ``ouster-cli source SENSOR diagnostics`` command to download sensor diagnostic dumps.
* [BREAKING] Merge the handling of ``--extrinsics-file`` ouster-cli option into ``--extrinsics`` option.
* [BREAKING] ouster-cli ``--extrinsics`` option requires adding double quotes for space separated values.
* ouster-cli ``--extrinsics`` option now accepts ``identity`` as a keyword for overrideng sensor extriniscs with identity.
* ouster-cli ``--extrinsics`` option now accepts the following additional formats besides the ``16`` numbers array format:
  * ``--extrinsics X,Y,Z,R,PY`` for position + euler angles.
  * ``--extrinsics X,Y,Z,QX,QY,QZ,QW`` for position + quaternion.
* Add cursor-driven AOI selection feature to 2d images in ouster-cli ``viz`` command.

ouster_osf
----------
* Introduce ``ouster::osf::AsyncWriter`` to offload saving ``LidarScan`` as OSF to a background thread, improving CLI performance when saving OSF files.
* Add the ``ouster::osf::Encoder`` type, which allows parameterizing the OSF compression level.
* Change the default OSF PNG encoder compression level to 1 from 4.
* [BREAKING] ``ouster.sdk.osf`` no longer exports lower-level OSF API classes (such as ``osf.Reader``.)
* ``ouster::osf::Writer::save`` now throws if the resolution of a ``LidarScan`` being saved doesn't match what is specified in sensor info/metadata.
* [BUGFIX] Fix incorrect ``OsfScanSource`` data when reading from an OSF file containing empty or missing streams.

ouster_viz
----------
* ``SimpleViz`` now drops frames when necessary to keep up with a live data source (i.e. sensor.)
* Add a map origin axis and label.
* Invoke frame buffer resize handlers added to ``PointViz`` when GLFW's window resize event fires.
* [BREAKING] Change ``PointViz::window_coordinates_to_image_pixel`` so that it always returns a pixel location (even outside the image), which can be useful in some situations.
* [BUGFIX] On screen display frame number starts at zero instead of one.
* [BUGFIX] ``LidarScanViz`` now only creates view modes for PIXEL fields.
* [BUGFIX] Use the last valid column pose as a ``LidarScan``'s origin, instead of the first.
* [BUGFIX] Limit number of keyboard shortcuts to toggle sensors from CTRL+1 to CTRL+9.
* [BUGFIX] Fix a key shortcut help rendering issue and improve consistency of key shortcut help.


[20241017] [0.13.1]
======================

* Add support for directly using IPv6 addresses for sensors in the CLI and in sensor clients.
* Typing '?' now displays the visualizer keyboard shortcuts in the visualizer window.
* Removed the ``async_client_example.cpp`` example.
* Un-deprecated ``ScanBatcher::ScanBatcher(size_t, const packet_format&)`` to remove a warning. (But please use ``ScanBatcher::ScanBatcher(const sensor_info&)`` instead.)

* [BREAKING] Removed the ``input_row_major`` parameter from the ``dewarp`` function. (``dewarp`` now infers the array type.)
* [BREAKING] Renamed ``DEFAULT_HTTP_REQUEST_TIMEOUT_SECONDS`` to ``LONG_HTTP_REQUEST_TIMEOUT_SECONDS``.
* [BREAKING] Changed the default value of ``LidarScanVizAccumulatorsConfig.accum_min_dist_num`` from ``1`` to ``0``.
* [BUGFIX] Fixed a visualizer glitch causing drawables not to render if added after a call to ``PointViz::update()`` but before ``PointViz::run()`` or ``PointViz::run_once()``.
* [BUGFIX] Fixed a visualizer crash when using ``HIGHLIGHT_SECOND`` mode with single-return datasets.
* [BUGFIX] Fixed an issue with the 2d images not updating when cycled during pause.
* [BUGFIX] Fixed a bug that the first scan pose it not identity when using slice slam command on a slam output osf file
* [BUGFIX] Re-introduce the RAW field option

Known Issues
------------

* Using an unbounded slice (e.g. with ``slice 100:`` during visualization can cause the source to loop back to the beginning (outside of the slice) when the source is a pcap file or an OSF saved with an earlier version of the SDK.
* A race condition in ``PointViz`` event handers occasionally causes a crash or unexpected results.


[20240702] [0.13.0]
======================

ouster_osf
------------------------
* Add full index of both receive and sensor timestamps to metadata
* Speed up opening of OSF files with index

* OSF now saves alert flags, thermal countdown and status, shot limiting countdown and status from ``LidarScan``.
* [BUGFIX] Fix OSF being unable to load LidarScans containing only custom fields
* [BUGFIX] Fix OSF not flushed when the user pressed CTRL-C more than once
* [BUGFIX] Fix improper timestamps when saving OSF on MacOS(m-series) and Windows
* [BUGFIX] Fix an issue with destaggering images after modifying ``SensorInfo`` in an ``OsfScanSource``.
* [BUGFIX] Fix an issue loading extrinsics from OSF metadata into a ``SensorInfo`` in ``OsfScanSource``.
* [BREAKING] Remove ``ChunksLayout`` and ``ChunkRef`` from Python API.

ouster_client/Python SDK
------------------------

* Add support for reading and writing ROS1 and ROS2 bag files.
* Add new sensor client interface ``ouster::sensor::SensorClient`` which supports multiple sensors as well as multiple sensors and IMU data on the same port
* Add higher level sensor client interface ```ouster::sensor::SensorScanSource`` which produces ``LidarScan`` s from multiple sensors
* Add ``ouster.sdk.client.SensorPacketSource`` which receives packets from multiple sensors
* Add support for multiple sensors to ``ouster.sdk.sensor.SensorScanSource``
* Greatly reduced redundant HTTP API calls to the sensor during initialization
* Deserialize FLAGS fields in each profile by default
* Add support for IPv6 multicast
* Add ``field_names`` argument to each scan source and to ``open_source`` to specify which fields to decode
* Add metadata validation functionality
* Add vendored json library
* Improved multi sensor pcap reading
* Improve ``ScanBatcher`` to release ``LidarScan`` as soon as they are completed
* ``ScanBatcher`` now adds alert flags, thermal countdown, and shot limiting countdown to ``LidarScan``.
* Use index to speed up ``ouster-cli source .osf info``
* Use index to speed up slicing of indexed OSF sources when sliced immediately after the ``source`` command
* Add ``LidarScan.get_first_valid_column_timestamp()``
* Add ``crc`` and ``calculate_crc`` methods to ``ouster::sensor::packet_format`` for obtaining or calculating (respectively) the CRC64 of a packet.
* ``scan_to_packets`` now creates packets with alert flags, thermal countdown and status, shot limiting countdown and status, and CRC64.
* Add ``ouster::pose_util::dewarp`` C++ function to de-warp a ``LidarScan`` (similar to ``ouster.sdk.pose_util`` in the Python API.)
* Add a constructor ``LidarScan(const ouster::sensor::sensor_info&)``.
* Always use ``nonstd::optional`` instead of drop-in ``std::optional`` from https://github.com/martinmoene/optional-lite.git to reduce issues associated with mixing C++14 and C++17.
* Add ``w()`` and ``h()`` methods to ``sensor_info`` in C++ and ``w`` and ``h`` properties to ``SensorInfo`` in Python.
* [BUGFIX] fix automatic UDP dest for FW 2.3 sensors.


* [BREAKING] Remove ``ouster::make_xyz_lut(const ouster::sensor::sensor_info&)``. (Use ``make_xyz_lut(const sensor::sensor_info& sensor, bool use_extrinsics)`` instead.)
* [BREAKING] changed REFLECTIVITY channel field size to 8 bits. (Important - this makes the SDK incompatible with FW 2.0.)
* [BREAKING] Removed ``UDPPacketSource`` and ``BufferedUDPSource``.
* [BREAKING] Removed ``ouster.sdk.util.firmware_version(hostname)`` please use ``ouster.sdk.client.SensorHttp.create(hostname).firmware_version()`` instead
* [BREAKING] ``open_source`` no longer automatically finds and applies extrinsics from ``sensor_extrinsics.json`` files. Use the ``extrinsics`` argument instead to specify the path to the relevant extrinsics file instead.
* [BREAKING] Deprecated ``osf.Scans(...)`` for ``osf.OsfScanSource(...).single_source(0)```.
* [BREAKING] Deprecated ``client.Sensor(...)`` for ``client.SensorPacketSource(...).single_source(0)```.
* [BREAKING] Deprecated ``pcap.Pcap(...)`` for ``pcap.PcapMultiPacketReader(...).single_source(0)```.
* [BREAKING] Deprecated ``ScanBatcher::ScanBatcher(size_t, const packet_format&)`` for ``ScanBatcher::ScanBatcher(const sensor_info&)``.
* [FUTURE BREAKING] Removing all instances of jsoncpp's ``Json::Value`` from the public C++ API methods in favor of ``std::string``.

ouster_viz
----------

* ``LidarScanViz`` now supports multi-sensor datasets.
* Add Python callback registration methods for mouse button and scroll events from ``PointViz``.
* Add Python and C++ callback registration methods for frame buffer resize events.
* Add ``MouseButton``, ``MouseButtonEvent``, and ``EventModifierKeys`` enums.
* Add methods ``aspect_ratio``, ``normalized_coordinates``, and ``window_coordinates`` to ``viz::WindowCtx``.
* Add method ``window_coordinates_to_image_pixel`` to ``viz::Image``. (See ``viz_events_example.cpp`` for an example.)
* Add ``current_camera()`` method to ``PointViz``.
* [BREAKING] ``SimpleViz`` no longer accepts a ``ScansAccumulator`` instance and now accepts scan/map accumulation parameters as keyword args in its constructor.
* [BREAKING] ``ScansAccumulator`` is split into several different classes: ``ScansAccumulator``, ``MapAccumulator``, ``TracksAccumulator``, and ``LidarScanVizAccumulators``.
* [BREAKING] changed ``PointViz`` mouse button callback to fire for both mouse button press and release events.
* [BREAKING] changed ``PointViz`` mouse button callback signature to use the new enums.
* [BREAKING] removed ``bool update_on_input()`` and ``update_on_input(bool)`` methods from ``PointViz``.
* [BUGFIX] SimpleViz throws a 'generator already executing' exception.

ouster-cli
----------

* Add support for reading and writing ROS1 and ROS2 bag files.
* Add support for working with multi scan sources.
* Add ``--fields`` argument to ``ouster-cli source`` to specify which fields to decode.
* Add metadata validation utility.
* [BUGFIX] Program doesn't terminate immediately when pressing CTRL-C the first time when streaming from a live sensor.
* [BUGFIX] Fix some errors that appeared when running ``ouster-cli util benchmark``
* [BREAKING] ``source`` no longer automatically finds and applies extrinsics from ``sensor_extrinsics.json`` files. Use the ``-E`` argument instead to specify the path to the relevant extrinsics file instead.
* [BREAKING] Moved raw recording functionality for BAG and PCAP to ``ouster-cli source ... record_raw`` command.
* [BREAKING] CLI plugins now need to handle a list of Optional[LidarScan] instead of a single LidarScan to support multi sources.

mapping
-------

* Update KissICP version from 0.4.0 to 1.0.0.
* Add multi-sensor support.

[20240702] [0.12.0]
===================

**Important: ouster-sdk installed from pypi now requires glibc >= 2.28.**

ouster_client/Python SDK
------------------------

* Add support for adding custom fields to ``LidarScan`` s with ``add_field`` and ``del_field``
* Added per-request timeout arguments to ``SensorHttp``
* Added sensor user_data to ``sensor_info/SensorInfo`` and metadata files
* Removed ``updated_metadata_string()`` and ``original_string()`` from ``sensor_info``
* Added ``to_json_string()`` to ``sensor_info`` to convert a ``sensor_info`` to a non-legacy
  metadata JSON string
* Unified Python and C++ ``Packet`` and ``PacketFormat`` classes
* Added ``validate`` function to ``LidarPacket`` and ``ImuPacket`` to check for ID and size mismatches
* [BREAKING] LidarScan's width and height have been switched to size_t from ptrdiff_t in C++
* Refactor metadata parsing
* Add ``get_version`` to ``sensor_info/SensorInfo`` to retrieve parsed version information
* Add ``get_product_info`` to ``sensor_info/SensorInfo`` to retrieve parsed lidar model information
* Raise an exception rather than throw an unrelated error when multiple viable metadata files are found for a given PCAP
* Add ability to slice a scan source, returning a new sliced ScanSource

* [BREAKING] Removed ``hostname`` in Python ``SensorInfo`` and ``name`` from C++ ``sensor_info``
* [BREAKING] Removed ``udp_port_lidar``, ``udp_port_imu`` and ``mode`` from C++ ``sensor_info``
* [BREAKING] Deprecated ``udp_port_lidar``, ``udp_port_imu`` and ``mode`` in Python ``SensorInfo``.
  These fields now point to the equivalent fields inside of ``SensorInfo::config``.
* [BREAKING] Removed ``cols`` and ``frequency`` from ``LidarMode`` in Python
* [BREAKING] Deprecated ```data``` and ``capture_timestamp`` from Python ``Packet``
* [BREAKING] Removed methods from Python ``ImuPacket`` and ``LidarPacket`` classes that simply wrapped ``PacketFormat``
* [BREAKING] Removed ``begin()`` and ``end()`` iterators of ``LidarScan`` in C++
* [BREAKING] Remove deprecated package stubs added in previous 0.11 release.
* [BREAKING] Replaced integer backed ``ChanField`` enumerations with strings.
* [BREAKING] Removed ``CUSTOM0`` through ``CUSTOM9`` ChanField enumerations.
* [BREAKING] Extra fields in sensor metadata are now ignored and discarded if saved from the resulting ``sensor_info/SensorInfo``

* [BUGFIX] Prevent last scan from being emitted twice for PCAP 
* [BUGFIX] Fix corrupted packets due to poor handling of fragmented packet drop in PCAPs
* [BUGFIX] Fix possible crash when working with custom UDPProfileLidars

ouster_viz
----------
* Support viewing custom ``LidarScan`` fields in viz
* Support viewing custom ``LidarScan`` 3 channel fields in viz as RGB

* [BUGFIX] Prevent OpenBLAS from using high amounts of CPU spin waiting

ouster_osf
----------

* Support saving custom ``LidarScan`` fields to OSF files

* [BREAKING] OsfWriter now takes in an optional list of fields to save rather than a list of fields and ChanFieldTypes to cast to

ouster-cli
----------

* Added support for slicing using time to ``ouster-cli source ... slice`` 
* Add sensor ``ouster-cli source ... userdata`` command to set and retrieve userdata on a sensor
* Add chainable ``ouster-cli source ... stats`` command
* Add chainable ``ouster-cli source ... clip`` command to discard points outside a provided range
* Add ``--rate max`` option to ``ouster-cli source ... viz```
* Improve argument naming and descriptions for ``ouster-cli source ... viz`` map and accum options:
  ``--accum-map`` is now called ``--map`` and ``--accum-map-ratio`` is now called ``--map-ratio``.
* New ``--map-size`` argument to set the maximum number of points used when ``--map`` is specified.

* [BUGFIX] Prevent dropped frames from live sensors by consuming scans as fast as they come in rather than sleeping

mapping
-------

* Move mapping into the sdk as ``ouster.sdk.mapping``
* Better handle looping while mapping
* Improve automatic downsample voxel size calculation

other
-----

* Updated VCPKG libraries to 2024.04.26

[20240510] [0.11.1]
===================

Important notes
---------------

* [BREAKING] the ``open_source`` method now returns a ``ScanSource`` by default, not a ``MultiScanSource``.

Python SDK
----------

* Updated the ``open_source`` documentation.
* Fixed an issue that caused the viz to redraw when the mouse cursor is moved.
* [BREAKING] The python slice ``[::]`` operator now returns a ``MultiScanSource`` / ``ScanSource``
  instead of a ``List``. Thus, invoking a ``scan_source[x:x+n]`` yields a fully functional scan source
  that is scoped to the range ``[x, x+n]``.
* [BREAKING] The python slice ``[::]`` operator no longer support negative step

ouster_client
-------------

* Improved the client initialization latency.

mapping
-------

* Fixed several issues with the documentation.


[20240425] [0.11.0]
===================

Important notes
---------------

* Dropped support for python3.7
* Dropped support macOS 10.15
* This will be the last release that supports Ubuntu 18.04.
* Moved all library level modules under ``ouster.sdk``, this includes ``ouster.client``, ``ouster.pcap``
  ``ouster.osf``. So the new access name will be ``ouster.sdk.client``, ``ouster.sdk.pcap`` and so on
* [BREAKING] many of the ``ouster-cli`` commands and arguments have changed (see below.)
* [BREAKING] moved ``configure_sensor`` method to ``ouster.sdk.sensor.util`` module
* [BREAKING] removed the ``pcap_to_osf`` method.


examples
--------

* Added a new ``async_client_example.cpp`` C++ example.


Python SDK
----------

* Add support for python 3.12, including wheels on pypi
* Updated VCPKG libraries to 2023.10.19
* New ``ScanSource`` API:

   * Added new ``MultiScanSource`` that supports streaming and manipulating LidarScan frames from multiple concurrent LidarScan sources

     * For non-live sources the ``MultiScanSource`` has the option to choose LidarScan(s) by index or choose a subset of scans using slicing operation
     * The ``MultiScanSource`` interface has the ability to fallback to ``ScanSource`` using the ``single_source(sensor_idx)``, ``ScanSource`` interface yield a single LidarScan on iteration rather than a List
     * The ``ScanSource`` interface obtained via ``single_source`` method supports same indexing and and slicing operations as the ``MultiScanSource``

  * Added a generic ``open_source`` that accepts sensor urls, or a path to a pcap recording or an osf file
  * Add explicit flag ``index`` to index unindexed osf files, if flag is set to ``True`` the osf file
    will be indexed and the index will be saved to the file on first attempt
  * Display a progress bar during index of pcap file or osf (if unindexed)

* Improved the robustness of the ``resolve_metadata`` method used to
  automatically identify the sensor metadata associated with a PCAP source.
* [bugfix] SimpleViz complains about missing fields
* [bugfix] Gracefully handle failed sensor connection attempts with proper error reporting
* [bugfix] Fix assertion error when using viz stepping on a live sensor
* [bugfix] Scope MultiLidarViz imports to viz commands
* [bugfix] LidarScan yielded with improper header/status
* [bugfix] OSF ScanSource fields property doesn't report the actual fields 
* Removed ``ouster.sdkx``, the ``open_source`` command is now part of ``ouster.sdk`` module
* The ``FLAGS`` field is always added to the list fields of any source type by default. In case of a 
  dual return lidar profile then a second ``FLAGS2`` will also be added. 


mapping
-------

* Updated SLAM API and examples.
* Added real time frame dropping capability to SLAM API.
* The ``ouster-mapping`` package now uses ``point-cloud-utils`` instead of ``open3d``.
* improved per-column pose accuracy, which is now based on the actual column timestamps


ouster-cli
----------

* Many commands can now be chained together, e.g. ``ouster-cli source <src> slam viz``.
* New ``save`` command can output the result in a variety of formats.
* Added ``--ts`` option for specifying the timestamps to use when saving an OSF
  file. Host packet receive time is the default, but not all scan sources have
  this info. Lidar packet timestamps can be used as an alternative.
* Changed the output format of ``ouster-cli discover`` to include more information.
* Added JSON format output option to ``ouster-cli discover``.
* Added a flag to output sensor user data to ``ouster-cli discover``.
* Update the minimum required version of ``zeroconf``.
* Removed ``python-magic`` package from required dependencies.
* Made the output of ``ouster-cli source <osf> info`` much more
  user-friendly. (``ouster-cli source <osf> dump`` gives old output.)
* [breaking] changed the argument format of the ``slice`` command.
* [breaking] removed the ``--legacy`` and ``--non-legacy`` flags.
* [breaking] removed the ``ouster-cli mapping``, ``ouster-cli osf``,
  ``ouster-cli pcap``, and ``ouster-cli sensor`` commands.
* [bugfix] return a nonzero exit code on error.
* [bugfix] fix an error that occurred when setting the IMU port using the
  ``-i`` option.


ouster_client
-------------

* Added a new buffered UDP source implementation ``BufferedUDPSource``.
* The method ``version_of_string`` is marked as deprecated, use ``version_from_string``
  instead.
* Added a new method ``firmware_version_from_metadata`` which works across firmwares.
* Added support for return order configuration parameter.
* Added support for gyro and accelerometer FSR configuration parameters.
* [bugfix] ``mtp_init_client`` throws a bad optional access.
* [bugfix] properly handle 32-bit frame IDs from the
  ``FUSA_RNG15_RFL8_NIR8_DUAL`` sensor UDP profile.


ouster_osf
----------

* [breaking] Greatly simplified OSF writer API with examples.
* [breaking] removed the ``to_native`` and ``from_native`` methods.
* Updated Doxygen API documentation for OSF C++ API.
* Removed support for the deprecated "standard" OSF file format. (The streaming
  OSF format is still supported.)
* Added ``osf_file_modify_metadata`` that allows updating the sensor info
  associated with each lidar stream in an OSF file.
* Warn the user if reading an empty or improperly indexed file.


ouster_viz
----------
* Added scaled palettes for calibrated reflectivity.
* Distance rings can now be hidden by setting their thickness to zero.
* [bugfix] Fix some rendering issues with the distance rings.
* [bugfix] Fix potential flickering in Viz


Known issues
------------

* ouster-cli discover may not provide info for sensors using IPv6 link-local
  networks on Python 3.8 or with older versions of zeroconf.
* ouster-cli when combining ``slice`` command with ``viz`` the program will
  exit once iterate over the selected range of scans even when
  the ``--on-eof`` option is set to ``loop``.

  - workaround: to have ``viz`` loop over the selected range, first perform a
    ``slice`` with ``save``, then playback the generated file.


[20231031] [0.10.0]
===================

Important notes
---------------

* This will be the last release that supports Python 3.7.
* This will be the last release that supports macOS 10.15.

ouster_viz
----------

* Added point cloud accumulation support
* Added an ``PointViz::fps()`` method to return the operating frame rate as a ``double``

ouster_client
-------------

* [BREAKING] Updates to ``sensor_info`` include:
    * new fields added: ``build_date``, ``image_rev``, ``prod_pn``, ``status``, ``cal`` (representing the value stored in the ``calibration_status`` metadata JSON key), ``config`` (representing the value of the ``sensor_config`` metadata JSON key)
    * the original JSON string is accessible via the ``original_string()`` method
    * The ``updated_metadata_string()`` now returns a JSON string reflecting any modifications to ``sensor_info``
    * ``to_string`` is now marked as deprecated
* [BREAKING] The RANGE field defined in `parsing.cpp`, for the low data rate profile, is now 32 bits wide (originally 16 bits.)
    * Please note this fixes a SDK bug. The underlying UDP format is unchanged.
* [BREAKING] The NEAR_IR field defined in `parsing.cpp`, for the low data rate profile, is now 16 bits wide (originally 8 bits.)
    * Plase note this fixes a SDK bug. The underlying UDP format is unchanged.
* [BREAKING] changed frame_id return size to 32 bits from 16 bits
* An array of per-packet timestamps (called ``packet_timestamp``) is added to ``LidarScan``
* The client now retries failed requests to an Ouster sensor's HTTP API
* Increased the default timeout for HTTP requests to 40s
* Added FuSA UDP profile to support Ouster FW 3.1+
* Improved ``ScanBatcher`` performance by roughly 3x (depending on hardware)
* Receive buffer size increased from 256KB to 1MB
* [bugfix] Fixed an issue that caused incorrect Cartesian point computation in the ``viz.Cloud`` Python class
* [bugfix] Fixed an issue that resulted in some ``packet_format`` methods returning an uninitialized value
* [bugfix] Fixed a libpcap-related linking issue
* [bugfix] Fixed an eigen 3.3-related linking issue
* [bugfix] Fixed a zero beam angle calculation issue
* [bugfix] Fixed dropped columns issue with 4096x5 and 2048x10

ouster-cli
----------

* Added ``source <FILE> slam`` and ``source <FILE> slam viz`` commands
* All metadata CLI options are changed to ``-m/--metadata``
* Added discovery for FW 3.1+ sensors
* Set signal multiplier by default in sensor/SOURCE sensor config
* use ``PYBIND11_MODULE`` instead of deprecated module constructor
* remove deprecated == in pybind for ``.is()``
* [bugfix] Fix report of fragmentation for ouster-cli pcap/SOURCE pcap info
* [bugfix] Fixed issue regarding windows mDNS in discovery
* [bugfix] Fixed cli pcap recording timestamp issue
* [BREAKING] CSV output ordering switched

ouster.sdk
----------

* ``ouster-mapping`` is now a required dependency
* [BREAKING] change the ``ouster.sdk.viz`` location to the ``ouster.viz``
  package, please update the references if you used ``ouster.sdk.viz`` module
* [bugfix] Fixed Windows pcap support for files larger than 2GB
* [bugfix] Fixed the order of ``LidarScan``'s ``w`` and ``h`` keyword arguments
* [bugfix] Fixed an issue with ``LidarPacket`` when using data recorded with older versions of Ouster Studio

Known issues
------------

* The dependency specifier for ``scipy`` is invalid per PEP-440
* ``get_config`` always returns true
* Repeated CTRL-C can cause a segmentation fault while visualizing a point cloud

20230710
========

* Update vcpkg ref of build to 2023-02-24

ouster_osf
----------

* Add Ouster OSF C++/Python library to save stream of LidarScans with metadata

ouster_client
-------------

* Add ``LidarScan.pose`` with poses per column
* Add ``_client.IndexedPcapReader`` and ``_client.PcapIndex`` to enable random pcap file access by frame number
* [BREAKING] remove ``ouster::Imu`` object
* [BREAKING] change the return type of ``ouster::packet_format::frame_id`` from ``uint16_t`` to ``uint32_t``
* [BREAKING] remove methods ``px_range``, ``px_reflectivity``, ``px_signal``, and ``px_ambient`` from ``ouster::packet_format``
* Add ``get_field_types`` function for ``LidarScan``, from ``sensor_info``
* bugfix: return metadata regardless of ``sensor_info`` status field
* Make timeout on curl more configurable
* [BREAKING] remove encoder_ticks_per_rev (deprecated)

ouster_viz
----------

* [BREAKING] Changed Python binding for ``Cloud.set_column_poses()`` to accept ``[Wx4x4]`` array
  of poses, column-storage order
* bugfix: fix label re-use
* Add ``LidarScan.pose`` handling to ``viz.LidarScanViz``, and new ``T`` keyboard
  binding to toggle column poses usage

ouster_pcap
-----------
* bugfix: Use unordered map to store stream_keys to avoid comparison operators on map

Python SDK
----------
* Add Python 3.11 wheels
* Retire simple-viz for ouster-cli utility
* Add default ? key binding to LidarScanViz and consolidate bindings into stored definition
* Remove pcap-to-csv for ouster-cli utility
* Add validator class for LidarPacket

ouster-cli
----------
This release also marks the introduction of the ouster-cli utility which includes, among many features:
* Visualization from a connected sensor with automatic configuration
* Recording from a connected sensor
* Simultaneous record and viz from a connected sensor
* Obtaining metadata from a connected sensor
* Visualization from a specified PCAP
* Slice, info, and conversion for a specificed PCAP
* Utilities for benchmarking system, printing system-info
* Discovery which indicates all connected sensors on network
* Automatic logging to .ouster-cli
* Mapping utilities


[20230403]
==========

* Default metadata output across all functionality has been switched to the non-legacy format

ouster_client
-------------
* Added a new method ``mtp_init_client`` to init the client with multicast support (experimental).
* the class ``SensorHttp``  which provides easy access to REST APIs of the sensor has been made public
  under the ``ouster::sensor::util`` namespace.
* breaking change: get_metadata defaults to outputting non-legacy metadata
* add debug five_word profile which will be removed later
* breaking change: remove deprecations on LidarScan

ouster_viz
----------
* update viz camera with other objects in draw

ouster_pcap
-----------
* add seek method to PcapReader
* add port guessing logic

python
------
* introduce utility to convert nonlegacy metadata to legacy
* use resolve_metadata to find unspecified metadata for simple-viz
* remove port guessing logic in favor of using new C++ ouster_pcap port guessing functionality
* add soft-id-check to skip the init_id/sn check for lidar_packets with metadata

Numerous changes to SimpleViz and LidarScanViz including:
* expose visible in viz to Python 
* introduce ImageMode and CloudMode
* bugfix: remove spurious sqrt application to autoleveled images


[20230114]
==========

ouster_client
--------------
* breaking change: signal multiplier type changed to double to support new FW values of signal
  multiplier.
* breaking change: make_xyz_lut takes mat4d beam_to_lidar_transform instead of
  lidar_origin_to_beam_origin_mm double to accomodate new FWs. Old reference Python implementation
  was kept, but new reference was also added.
* address an issue that could cause the processed frame being dropped in favor or the previous
  frame when the frame_id wraps-around.
* added a new flag ``CONFIG_FORCE_REINIT`` for ``set_config()`` method, to force the sensor to reinit
  even when config params have not changed.
* breaking change: drop defaults parameters from the shortform ``init_client()`` method.
* added a new method ``init_logger()`` to provide control over the logs emitted by ``ouster_client``.
* add parsing for new FW 3.0 thermal features shot_limiting and thermal_shutdown statuses and countdowns
* add frame_status to LidarScan
* introduce a new method ``cartesianT()`` which speeds up the computation of point projecion from range
  image, the method also can process the cartesian product with single float precision. A new unit test
  ``cartesian_test`` which shows achieved speed up gains by the number of valid returns in lidar scan.
* add ``RAW_HEADERS`` ChanField to LidarScan for packing headers and footer (alpha version, may be
  changed/removed without notice in the future)

python
------
* breaking change: drop defaults parameters of ``client.Sensor`` constructor.
* breaking change: change Scans interface Timeout to default to 1 second instead of None
* added a new method ``init_logger()`` to provide control over the logs emitted by ``client.Sensor``.
* add ``client.LidarScan`` methods ``__repr__()`` and ``__str__()``.
* changed default timeout from 1 seconds to 2 seconds

ouster_viz
----------
* add ``SimpleViz.screenshot()`` function and a key handler ``SHIFT-Z`` to
  save a screenshot. Can be called from a client thread, and executes
  asyncronously (i.e. returns immediately and pushes a one off callback
  to frame buffer handlers)
* add ``PointViz.viewport_width()`` and ``PointViz.viewport_height()`` functions
* add ``PointViz.push/pop_frame_buffer_handler()`` to attach a callbacks on
  every frame draw update that calls from the main rendering loop.
* add ``SHIFT-X`` key to SimpleViz to start continuous saving of screenshots
  on every draw operation. (good for making videos)
* expose ``Camera.set_target`` function through pybind

ouster-sdk
----------
* Moved ouster_ros to its own repo
* pin ``openssl`` Conan package dependency to ``openssl/1.1.1s`` due to
  ``libtins`` and ``libcurl`` conflicting ``openssl`` versions


[20220927]
==========

ouster_client
--------------
* fix a bug in longform ``init_client()`` which was not setting timestamp_mode and lidar_mode correctly
  

[20220826]
==========

* drop support for buliding C++ libraries and Python bindings on Ubuntu 16.04
* drop support for buliding C++ libraries and Python bindings on Mac 10.13, Mac 10.14
* Python 3.6 wheels are no longer built and published
* drop support for sensors running FW < 2.0
* require C++ 14 to build

ouster_client
--------------
* add ```CUSTOM0-9`` ChanFields to LidarScan object
* fix parsing measurement status from packets: previously, with some UDP profiles, higher order bits
  could be randomly set
* add option for EIGEN_MAX_ALIGN_BYTES, ON by default
* use of sensor http interface for comms with sensors for FW 2.1+
* propogate C++ 17 usage requirement in cmake for C++ libraries built as C++17
* allow vcpkg configuration via environment variables
* fix a bug in sensor_config struct equality comparison operator

ouster_viz
----------
* clean up GL context logic to avoid errors on window/intel UHD graphics

python
------
* windows extension modules are now statically linked to avoid potential issues with vendored dlls

ouster_ros
----------
* drop ROS kinetic support
* switch from nodes to nodelets
* update topic names, group under single ros namespace
* separate launch files for play, replay, and recording
* drop FW 1.13 compatibility for sensors and recorded bags
* remove setting of EIGEN_MAX_ALIGN_BYTES
* add two new ros services /ouster/get_config and /ouster/set_config (experimental)
* add new timestamp_mode TIME_FROM_ROS_TIME
* declare PCL_NO_PRECOMPILE ahead of all PCL library includes


[20220608]
==========

ouster_client
-------------
* change single return parsing for FW 2.3.1

python
------
* single return parsing for FW 2.3.1 reflects change from ouster_client


[20220504]
==========

* update supported vcpkg tag to 2022.02.23
* update to manylinux2014 for x64 linux ``ouster-sdk`` wheels
* Ouster SDK documentation overhaul with C++/Python APIs in one place
* sample data updated to firmware 2.3

ouster_client
-------------
* fix the behavior of ``BeamUniformityCorrector`` on azimuth-windowed data by ignoring zeroed out
  columns
* add overloads in ``image_processing.h`` to work with single-precision floats
* add support for new ``RNG19_RFL8_SIG16_NIR16`` single-return and ``RNG15_RFL8_NIR8`` low-bandwidth
  lidar UDP profiles introduced in firmware 2.3

ouster_viz
----------
* switch to glad for OpenGL loading. GLEW is still supported for developer builds
* breaking change: significant API update of the ``PointViz`` library. See documentation for details
* the ``simple_viz`` example app and ``LidarScanViz`` utility have been removed. Equivalent
  functionality is now provided via Python
* add basic support for drawing 2d and 3d text labels
* update to OpenGL 3.3

python
------
* fix a bug where incorrectly sized packets read from the network could cause the client thread to
  silently exit, resulting in a timeout
* fix ``client.Scans`` not raising a timeout when using the ``complete`` flag and receiving only
  incomplete scans. This could cause readings scans to hang in rare situations
* added bindings for the new ``PointViz`` API and a new module for higher-level visualizer utilities
  in ``ouster.sdk.viz``. See API documentation for details
* the ``ouster-sdk`` package now includes an example visualizer, ``simple-viz``, which will be
  installed on that path for the Python environment

ouster_ros
-----------
* support new fw 2.3 profiles by checking for inclusion of fields when creating point cloud. Missing
  fields are filled with zeroes

[20220107]
==========

* add support for arm64 macos and linux. Releases are now built and tested on these platforms
* add support for Python 3.10
* update supported vcpkg tag to 2021.05.12
* add preliminary cpack and install support. It should be possible to use a pre-built SDK package
  instead of including the SDK in the build tree of your project

ouster_client
-------------
* update cmake package version to 0.3.0
* avoid unnecessary DNS lookup when using numeric addresses with ``init_client()``
* disable collecting metadata when sensor is in STANDBY mode
* breaking change: ``set_config()`` will now produce more informative errors by throwing
  ``std::invalid_argument`` with an error message when config parameters fail validation
* use ``SO_REUSEPORT`` for UDP sockets on non-windows platforms
* the set of fields available on ``LidarScan`` is now configurable. See the new ``LidarScan``
  constructors for details
* added ``RANGE2``, ``SIGNAL2`` and ``REFLECTIVITY2`` channel fields to support handling data from
  the second return
* ``ScanBatcher`` will now parse and populate only the channel fields configured on the
  ``LidarScan`` passed to ``operator()()``
* add support for new configuration parameters: ``udp_profile_lidar``, ``udp_profile_imu`` and
  ``columns_per_packet``
* add udp ports, the new initialization id field, and udp profiles to the metadata stored in
  the ``sensor_info`` struct
* ``sensor_info::name`` is now deprecated and will stop being populated in the future
* add methods to query and iterate over available ``LidarScan`` fields and field types
* breaking change: removed ``LidarScan::block`` and ``LidarScan::data`` members. These can't be
  supported for different packet profiles
* the ``LidarScan::Field`` defniition has been moved to ``sensor::ChanField`` and enumerators have
  been renamed to match the sensor user manual. The old names are still available, but deprecated
* deprecate accessing encoder values and frame ids from measurement blocks using ``packet_format``
  as these will not be reported by the sensor in some future configurations
* add ``packet_frame_id`` member function to ``packet_format``
* add ``col_field`` member function to ``packet_format`` for parsing channel field values for an
  entire measurement block
* add new accessors for measurement headers to ``LidarScan``, deprecating the existing ``header``
  member function
* represent empty sensor config with an empty object instead of null in json representation of the
  ``sensor_config`` datatype
* update cmake package version to 0.2.1
* add a conservative socket read timeout so ``init_client()`` will fail with an error message when
  another client fails to close a TCP connection (addresses #258)
* when passed an empty string for the ``udp_dest_host`` parameter, ``init_client()`` will now
  configure the sensor using ``set_udp_dest_auto``. Previously, this would turn off UDP output on
  the sensor, so any attempt to read data would time out (PR #255)
* fall back to binding ipv4 UDP sockets when ipv6 is not available (addresses #261)

ouster_pcap
-----------
* report additional information in the ``packet_info`` struct and remove separate ``stream_info``
  API
* switch the default pcap encapsulation to ethernet for Ouster Studio compatibility (addresses #265)

ouster_ros
----------
* update ROS package version to 0.3.0
* allow setting the packet profile in ouster.launch with the ``udp_profile_lidar`` parameter
* publish additional cloud and image topics for the second return when running in dual returns mode
* fix ``os_node`` crash on shutdown due to Eigen alignment flag not being propogated by catkin
* update ROS package version to 0.2.1
* the ``udp_dest`` parameter to ouster.launch is now optional when connecting to a sensor

ouster_viz
----------
* the second CLI argument of simple_viz specifying the UDP data destination is now optional
* fixed bug in AutoExposure causing more points to be mapped to near-zero values
* add functionality to display text over cuboids

python
------
* update ouster-sdk version to 0.3.0
* improve heuristics for identifying sensor data in pcaps, including new packet formats
* release builds for wheels on Windows now use the VS 2017 toolchain and runtime (previously 2019)
* fix potential use-after-free in ``LidarScan.fields``
* update ouster-sdk version to 0.3.0b1
* return an error when attempting to initialize ``client.Sensor`` in STANDBY mode
* check for errors while reading from a ``Sensor`` packet source and waiting for a timeout. This
  should make stopping a process with ``SIGINT`` more reliable
* add PoC bindings for the ``ouster_viz`` library with a simple example driver. See the
  ``ouster.sdk.examples.viz`` module
* add bindings for new configuration and metadata supported by the client library
* breaking change: the ``ChanField`` enum is now implemented as a native binding for easier interop
  with C++. Unlike Python enums, the bound class itself is no longer sized or iterable. Use
  ``ChanField.values`` to iterate over all ``ChanField`` values or ``LidarScan.fields`` for fields
  available on a particular scan instance
* breaking change: arrays returned by ``LidarPacket.field`` and ``LidarPacket.header`` are now
  immutable. Modifying the underlying packet buffer through these views was never fully supported
* deprecate ``ColHeader``, ``LidarPacket.header``, and ``LidarScan.header`` in favor of new
  properties: ``timestamp``, ``measurement_id``, ``status``, and ``frame_id``
* replace ``LidarScan`` with native bindings implementing the same API
* ``xyzlut`` can now accept a range image as an ndarray, not just a ``LidarScan``
* update ouster-sdk version to 0.2.2
* fix open3d example crash on exit when replaying pcaps on macos (addresses #267)
* change open3d normalization to use bound AutoExposure


[20210608]
==========

ouster_client
-------------
* update cmake package version to 0.2.0
* add support for new signal multiplier config parameter
* add early version of a C++ API covering the full sensor configuration interface
* increase default initialization timeout to 60 seconds to account for the worst case: waking up
  from STANDBY mode

ouster_pcap
-----------
* ``record_packet()`` now requires passing in a capture timestamp instead of using current time
* work around libtins issue where capture timestamps for pcaps recorded on Windows are always zero
* add preliminary C++ API for working with pcap files containing a single sensor packet capture

ouster_ros
----------
* update ROS package version to 0.2.0
* add Dockerfile to easily set up a build environment or run nodes
* ``img_node`` now outputs 16-bit images, which should be more useful. Range image output is now in
  units of 4mm instead of arbitrary scaling (addresses #249)
* ``img_node`` now outputs reflectivity images as well on the ``reflec_image`` topic
* change ``img_node`` topics to match terminology in sensor documentation: ``ambient_image`` is now
  ``nearir_image`` and ``intensity_image`` is now ``signal_image``
* update rviz config to use flat squares by default to work around `a bug on intel systems
  <https://github.com/ros-visualization/rviz/issues/1508>`_
* remove viz_node and all graphics stack dependencies from the package. The ``viz`` flag on the
  launch file now runs rviz (addresses #236)
* clean up package.xml and ensure that dependencies are installable with rosdep (PR #219)
* the ``metadata`` argument to ouster_ros launch file is now required. No longer defaults to a name
  based on the hostname of the sensor

ouster_viz
----------
* update reflectivity visualization for changes in the upcoming 2.1 firmware. Add new colormap and
  handle 8-bit reflectivity values
* move most of the visualizer code out of public headers and hide some implementation details
* fix visualizer bug causing a small viewport when resizing the window on macos with a retina
  display

python
------
* update ouster-sdk version to 0.2.1
* fix bug in determining if a scan is complete with single-column azimuth windows
* closed PacketSource iterators will now raise an exception on read
* add examples for visualization using open3d (see: ``ouster.sdk.examples.open3d``)
* add support for the new signal multiplier config parameter
* preserve capture timestamps on packets read from pcaps
* first release: version 0.2.0 of ouster-sdk. See the README under the ``python`` directory for
  details and links to documentation


[20201209]
==========

Changed
-------

* switched to date-based version scheme. No longer tracking firmware versions
* added a top-level ``CMakeLists.txt``. Client and visualizer should no longer be built
  separately. See the README for updated build instructions
* cmake cleanup, including using custom "find modules" to provide better compatibility between
  different versions of cmake
* respect standard cmake ``BUILD_SHARED_LIBS`` and ``CMAKE_POSITION_INDEPENDENT_CODE`` flags
* make ``ouster_ros`` easier to use as a dependency by bundling the client and viz libraries
  together into a single library that can be used through catkin
* updated client example code. Now uses more of the client APIs to capture data and write to a
  CSV. See ``ouster_client/src/example.cpp``
* replace callback-based ``batch_to_scan`` function with ``ScanBatcher``. See ``lidar_scan.h`` for
  API docs and the new client example code
* update ``LidarScan`` API. Now includes accessors for measurement blocks as well as channel data
  fields. See ``lidar_scan.h`` for API docs
* add client version field to metadata json, logs, and help text
* client API renaming to better reflect the Sensor Software Manual


[1.14.0-beta.14] - 2020-08-27
=============================

Added
-----

* support for ROS noetic in ``ouster_ros``. Note: this may break building on very old platforms
  without a C++14-capable compiler
* an extra extrinsics field in ``sensor_info`` for conveniently passing around an extra user-supplied
  transform
* a utility function to convert ``lidar_scan`` data between the "staggered" representation where each
  column has the same timestamp and "de-staggered" representation where each column has the same
  azimuth angle
* mask support in the visualizer library in ``ouster_viz``

Changed
-------

* ``ouster_ros`` now requires C++14 to support building against noetic libraries
* replaced ``batch_to_iter`` with ``batch_to_scan``, a simplified function that writes directly to a
  ``lidar_scan`` instead of arbitrary iterator

Fixed
-----

* ipv6 support using dual-stack sockets on all supported platforms. This was broken since the
  beta.10 release
* projection to Cartesian coordinates now takes into account the vertical offset the sensor and
  lidar frames
* the reference frame of point cloud topics in ``ouster_ros`` is now correctly reported as the "sensor
  frame" defined in the user guide


[1.14.0-beta.12] - 2020-07-10
=============================

*no changes*


[1.14.0-beta.11] - 2020-06-17
=============================

*no changes*


[1.14.0-beta.10] - 2020-05-21
=============================

Added
-----

* preliminary support for Windows and macOS for ``ouster_viz`` and ``ouster_client``

Changed
-------

* replaced VTK visualizer library with one based on GLFW
* renamed all instances of "OS1" including namespaces, headers, node and topic names, to reflect
  support for other product lines
* updated all xyz point cloud calculations to take into account new ``lidar_origin_to_beam_origin``
  parameter reported by sensors
* client and ``os_node`` and ``simple_viz`` now avoid setting the lidar and timestamp modes when
  connecting to a client unless values are explicitly specicified

Fixed
-----

* increase the UDP receive buffer size in the client to reduce chances of dropping packets on
  platforms with low defaults
* ``os_cloud_node`` output now uses the updated point cloud calculation, taking into account the lidar
  origin offset
* minor regression with destaggering in img_node output in previous beta


[1.14.0-beta.4] - 2020-03-17
============================

Added
-----

* support for gen2 hardware in client, visualizer, and ROS sample code
* support for updated "packed" lidar UDP data format for 16 and 32-beam devices with firmware 1.14
* range markers in ``simple_viz`` and ``viz_node``. Toggle display using ``g`` key. Distances can be
  configured from ``os1.launch``.
* post-processing to improve ambient image uniformity in visualizer

Changed
-------

* use random ports for lidar and imu data by default when unspecified


[1.13.0] - 2020-03-16
=====================

Added
-----

* post-processing to improve ambient image uniformity in visualizer
* make timestamp mode configurable via the client (PR #97)

Changed
-------

* turn on position-independent code by default to make using code in libraries easier (PR #65)
* use random ports for lidar and imu data by default when unspecified

Fixed
-----

* prevent legacy tf prefix from making invalid frame names (PR #56)
* use ``iterator_traits`` to make ``batch_to_iter`` work with more types (PR #70)
* use correct name for json dependency in ``package.xml`` (PR #116)
* handle udp socket creation error gracefully in client


[1.12.0] - 2019-05-02
=====================

Added
-----

* install directives for ``ouster_ros`` build (addresses #50)

Changed
-------

* flip the sign on IMU acceleration output to follow usual conventions
* increase the update rate in the visualizer to ~60hz

Fixed
-----

* visualizer issue where the point cloud would occasionally occasionally not be displayed using
  newer versions of Eigen


[1.11.0] - 2019-03-26
=====================

Added
-----

* allow renaming tf ids using the ``tf_prefix`` parameter

Changed
-------

* use frame id to batch packets so client code deals with reordered lidar packets without splitting
  frames
* use a uint32_t for PointOS1 timestamps to avoid unnecessary loss of precision

Fixed
-----

* bug causing ring and reflectivity to be corrupted in os1_cloud_node output
* misplaced sine in azimuth angle calculation (addresses #42)
* populate timestamps on image node output (addresses #39)


[1.10.0] - 2019-01-27
=====================

Added
-----

* ``os1_node`` now queries and uses calibrated beam angles from the sensor
* ``os1_node`` now queries and uses imu / lidar frames from the sensor
* ``os1_node`` reads and writes metadata to ``${ROS_HOME}`` to support replaying data with calibration
* ROS example code now publishes tf2 transforms for imu / lidar frames (addresses #12)
* added ``metadata`` parameter to ``os1.launch`` to override location of metadata
* added ``viz`` parameter to ``os1.launch`` to run the example visualizer with ROS
* added ``image`` parameter to ``os1.launch`` to publish image topics to rviz (addresses #21)
* added range field to ``PointOS1``

Changed
-------

* split point-cloud publishing out of ``os1_node`` into ``os1_cloud_node``
* example visualizer controls:

  - press ``m`` to cycle through color modes instead of ``i``, ``z``, ``Z``, ``r``
  - ``r`` now resets the camera position
  - range/signal images automatically resized to fit window height

* updated OS-1 client to use newer TCP configuration commands
* updated OS-1 client to set the requested lidar mode, reinitialize on connection
* changed point cloud batching to be based on angle rather than scan duration
* ``ouster_client`` now depends on the ``jsoncpp`` library
* switched order of fields in ``PointOS1`` to be compatible with ``PointXYZI`` (addresses #16)
* moved example visualizer VTK rendering into the main thread (merged #23)
* the timestamp field of PointOS1 now represents time since the start of the scan (the timestamp of
  the PointCloud2 message) in nanoseconds

Removed
-------

* removed keyboard camera controls in example visualizer
* removed panning and rotating of the image panel in example visualizer

Fixed
-----

* no longer dropping UDP packets in 2048 mode on tested hardware
* example visualizer:

  - point cloud display focus no longer snaps back on rotation
  - fixed clipping issues with parallel projection
  - fixed point coloring issues in z-color mode
  - improved visualizer performance

