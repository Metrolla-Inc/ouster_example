from typing import Iterator, List, Optional, Union, Protocol
from ouster.sdk._bindings.client import SensorInfo, LidarScan
from .data import FieldTypes
from .scan_source import ScanSource
import numpy as np


class MultiScanSource(Protocol):
    """Represents only data stream from more than one source."""

    @property
    def sensors_count(self) -> int:
        """Number of individual scan streams that this scan source holds."""
        ...

    @property
    def metadata(self) -> List[SensorInfo]:
        """A list of 'SensorInfo' objects associated with every scan streams."""
        ...

    @property
    def is_live(self) -> bool:
        """True if data obtained from the RUNNING sensor or as a stream from the socket

        Returns:
            True if data obtained from the RUNNING sensor or as a stream from the socket
            False if data is read from a stored media. Restarting an ``iter()`` means that
                    the data can be read again.
        """
        ...

    @property
    def is_seekable(self) -> bool:
        """True for non-live sources, This property can be True regardless of scan source being indexed or not.
        """
        ...

    @property
    def is_indexed(self) -> bool:
        """True for IndexedPcap and OSF scan sources, this property tells users whether the underlying source
        allows for random access of scans, see __getitem__.
        """
        ...

    @property
    def field_types(self) -> List[FieldTypes]:
        """Field types are present in the LidarScan objects on read from iterator"""
        ...

    @property
    def fields(self) -> List[List[str]]:
        """Fields are present in the LidarScan objects on read from iterator"""
        ...

    @property
    def scans_num(self) -> List[Optional[int]]:
        """Number of scans available, in case of a live sensor or non-indexable scan source
         this method returns a None for that stream"""
        ...

    def __len__(self) -> int:
        """returns the number of scans containe with the scan_source, in case scan_source has
        more than one sensor then this would measure the number of collated scans across the
        streams in the case of a live sensor or non-indexable scan source this method throws
        a TypeError exception"""
        ...

    def __iter__(self) -> Iterator[List[Optional[LidarScan]]]:
        ...

    def _seek(self, key: int) -> None:
        """seek/jump to a specific item within the list of LidarScan objects that this particular scan
        source has access to"""
        ...

    def __getitem__(self, key: Union[int, slice]
                    ) -> Union[List[Optional[LidarScan]], 'MultiScanSource']:
        """Indexed access and slices support"""
        ...

    def close(self) -> None:
        """Manually release any underlying resource."""
        ...

    def __del__(self) -> None:
        """Automatic release of any underlying resource."""
        ...

    def single_source(self, stream_idx: int) -> ScanSource:
        from .scan_source_adapter import ScanSourceAdapter
        return ScanSourceAdapter(self, stream_idx)

    def _slice_iter(self, key: slice) -> Iterator[List[Optional[LidarScan]]]:
        ...

    def slice(self, key: slice) -> 'MultiScanSource':
        """Constructs a MultiScanSource matching the specificed slice"""
        from ouster.sdk.util.forward_slicer import ForwardSlicer
        from ouster.sdk.client.multi_sliced_scan_source import MultiSlicedScanSource
        L = len(self)
        k = ForwardSlicer.normalize(key, L)
        if k.step < 0:
            raise TypeError("slice() can't work with negative step")
        return MultiSlicedScanSource(self, k)

    def clip(self, fields: List[str], lower: int, upper: int) -> 'MultiScanSource':
        """Constructs a MultiScanSource matching the specificed clip options"""
        from ouster.sdk.client.multi_clipped_scan_source import MultiClippedScanSource
        return MultiClippedScanSource(self, fields, lower, upper)

    def reduce(self, beams: List[int]) -> 'MultiScanSource':
        """Constructs a reduced MultiScanSource matching the beam count"""
        from ouster.sdk.client.multi_reduced_scan_source import MultiReducedScanSource
        return MultiReducedScanSource(self, beams)

    def mask(self, fields: List[str], masks: List[Optional[np.ndarray]]) -> 'MultiScanSource':
        """Constructs a masked MultiScanSource"""
        from ouster.sdk.client.multi_masked_scan_source import MultiMaskedScanSource
        return MultiMaskedScanSource(self, fields, masks)
