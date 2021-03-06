///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2014 Final Level
// Author: Denys Misko <gdraal@gmail.com>
// Distributed under BSD (3-Clause) License (See
// accompanying file LICENSE)
//
// Description: Metis storage server control class implementation
///////////////////////////////////////////////////////////////////////////////

#include "storage.hpp"
#include "metis_log.hpp"

using namespace fl::metis;

Storage::Storage(const char *path, const double minFree, const TSize maxSliceSize)
	: _sliceManager(path, minFree, maxSliceSize)
{
	if (!_sliceManager.loadIndex(_index)) {
		log::Fatal::L("Can't load index\n");
		throw std::exception();
	}
}

bool Storage::add(const char *data, const ItemHeader &itemHeader)
{
	IndexEntry ie;
	ie.header = itemHeader;
	if (!_sliceManager.add(data, ie)) {
		log::Fatal::L("Can't add an object to the slice manager\n");
		return false;
	}
	_index.add(ie);
	return true;
}

bool Storage::add(const ItemHeader &itemHeader, File &putTmpFile, BString &buf)
{
	IndexEntry ie;
	ie.header = itemHeader;
	if (!_sliceManager.add(putTmpFile, buf, ie)) {
		log::Fatal::L("Can't add an object to the slice manager\n");
		return false;
	}
	_index.add(ie);
	return true;
}

bool Storage::remove(const ItemHeader &itemHeader)
{
	Range::Entry entry;
	if (!_index.find(itemHeader.rangeID, itemHeader.itemKey, entry))
		return true;
	
	if (entry.timeTag <= itemHeader.timeTag) {
		if (_sliceManager.remove(itemHeader, entry.pointer)) {
			_index.remove(itemHeader);
			return true;
		} else {
			log::Fatal::L("Can't delete an object from the slice manager\n");
			return false;
		}
	} else {
		log::Error::L("Can't delete an newer object\n");
		return false;
	}
}

bool Storage::findAndFill(const ItemIndex &itemIndex, ItemInfo &itemInfo)
{
	Range::Entry entry;
	if (!_index.find(itemIndex.rangeID, itemIndex.itemKey, entry))
		return false;	
	itemInfo.index = itemIndex;
	itemInfo.size = entry.size;
	itemInfo.timeTag = entry.timeTag;
	return true;
}

bool Storage::get(const GetItemChunkRequest &itemRequest, BString &data)
{
	Range::Entry entry;
	if (!_index.find(itemRequest.rangeID, itemRequest.itemKey, entry))
		return false;	
	if (entry.size == 0)
		return false;
	if ((itemRequest.seek + itemRequest.chunkSize) > entry.size) {
		log::Warning::L("Storage::get: Seek %u out of range %u\n", itemRequest.seek + itemRequest.chunkSize, entry.size);
		return false;
	}
	return _sliceManager.get(data, entry.pointer, itemRequest.seek, itemRequest.chunkSize);
}

bool Storage::ping(StoragePingAnswer &storageAnswer)
{
	return _sliceManager.ping(storageAnswer);
}

bool Storage::getRangeItems(const TRangeID rangeID, BString &data)
{
	return _index.getRangeItems(rangeID, data);
}
