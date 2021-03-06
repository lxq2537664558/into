/* This file is part of Into.
 * Copyright (C) Intopii 2013.
 * All rights reserved.
 *
 * Licensees holding a commercial Into license may use this file in
 * accordance with the commercial license agreement. Please see
 * LICENSE.commercial for commercial licensing terms.
 *
 * Alternatively, this file may be used under the terms of the GNU
 * Affero General Public License version 3 as published by the Free
 * Software Foundation. In addition, Intopii gives you special rights
 * to use Into as a part of open source software projects. Please
 * refer to LICENSE.AGPL3 for details.
 */

#include "PiiMatrix.h"

PiiMatrixData* PiiTypelessMatrix::createReference(int rows, int columns, void* buffer) const
{
  PiiMatrixData* pData = PiiMatrixData::createReferenceData(rows, columns,
                                                            d->iStride,
                                                            buffer);
  d->reserve();
  pData->pSourceData = d;
  return pData;
}

void* PiiTypelessMatrix::appendRow(std::size_t bytesPerRow)
{
  if (d->iRows >= d->iCapacity)
    reserve(qBound(1, d->iRows * 2, d->iRows + 1024), bytesPerRow);
  return d->row(d->iRows++);
}

void* PiiTypelessMatrix::insertRow(int index, std::size_t bytesPerRow)
{
  if (index == -1 || index == d->iRows)
    return appendRow(bytesPerRow);

  // If we are not the sole user of the data, need to reallocate.
  if (d->iRefCount != 1 ||
      d->bufferType != PiiMatrixData::InternalBuffer)
    {
      PiiMatrixData* pData = PiiMatrixData::createUninitializedData(d->iRows + 1,
                                                                    d->iColumns,
                                                                    bytesPerRow);
      for (int r=index; r<d->iRows; ++r)
        std::memcpy(pData->row(r+1), d->row(r), bytesPerRow);
      d->release();
      d = pData;
    }
  else
    {
      if (d->iRows + 1 > d->iCapacity)
        d = PiiMatrixData::reallocate(d, d->iRows + 1);
      for (int r=d->iRows; r>index; --r)
        std::memcpy(d->row(r), d->row(r-1), bytesPerRow);
      ++d->iRows;
    }
  return d->row(index);
}

void PiiTypelessMatrix::removeRow(int index, std::size_t bytesPerRow)
{
  if (index != -1)
    for (int i=index; i<d->iRows-1; ++i)
      std::memcpy(d->row(i), d->row(i+1), bytesPerRow);
  if (d->iRows > 0)
    --d->iRows;
}

void PiiTypelessMatrix::removeRows(int index, int cnt, std::size_t bytesPerRow)
{
  int iRowsToMove = d->iRows - index - cnt;
  if (iRowsToMove > 0)
    std::memmove(d->row(index), d->row(index + cnt), iRowsToMove * bytesPerRow);
  d->iRows -= cnt;
}

void PiiTypelessMatrix::removeColumn(int index, std::size_t bytesPerItem)
{
  removeColumns(index, 1, bytesPerItem);
}

void PiiTypelessMatrix::removeColumns(int index, int cnt, std::size_t bytesPerItem)
{
  if (index != -1)
    {
      std::size_t iRowEndBytes = (d->iColumns - index - cnt) * bytesPerItem;
      char* pRowEnd = static_cast<char*>(d->row(0)) + index * bytesPerItem;
      for (int r=0; r<d->iRows; ++r, pRowEnd += d->iStride)
        std::memmove(pRowEnd, pRowEnd + cnt * bytesPerItem, iRowEndBytes);
    }
  if (d->iColumns > 0)
    d->iColumns -= cnt;
}

void* PiiTypelessMatrix::appendColumn(std::size_t bytesPerItem)
{
  int iRows = d->iRows;
  resize(qMax(d->iCapacity, iRows), d->iColumns + 1, bytesPerItem);
  d->iRows = iRows;
  return static_cast<char*>(d->row(0)) + bytesPerItem * (d->iColumns - 1);
}

void* PiiTypelessMatrix::insertColumn(int index, std::size_t bytesPerItem)
{
  if (index == -1 || index == d->iColumns)
    return appendColumn(bytesPerItem);

  std::size_t iRowEndBytes = (d->iColumns - index) * bytesPerItem;
  std::size_t iBytesPerRow = (d->iColumns + 1) * bytesPerItem;

  // If we are not the sole user of the data or the current array
  // cannot take any more, need to reallocate.
  if (d->iRefCount != 1 ||
      d->bufferType != PiiMatrixData::InternalBuffer ||
      iBytesPerRow > d->iStride)
    {
      PiiMatrixData* pData = PiiMatrixData::createUninitializedData(qMax(d->iRows, d->iCapacity),
                                                                    d->iColumns + 1,
                                                                    iBytesPerRow);
      pData->iRows = d->iRows;
      std::size_t iRowStartBytes = index * bytesPerItem;
      char* pTargetRow = static_cast<char*>(pData->row(0));
      const char* pSourceRow = static_cast<char*>(d->row(0));
      for (int r=d->iRows; r--; pSourceRow += d->iStride, pTargetRow += pData->iStride)
        {
          std::memcpy(pTargetRow, pSourceRow, iRowStartBytes);
          std::memcpy(pTargetRow + iRowStartBytes + bytesPerItem, pSourceRow + iRowStartBytes, iRowEndBytes);
        }
      d->release();
      d = pData;
    }
  else
    {
      char* pRowEnd = static_cast<char*>(d->row(0)) + index * bytesPerItem;
      for (int r=d->iRows; r--; pRowEnd += d->iStride)
        std::memmove(pRowEnd + bytesPerItem, pRowEnd, iRowEndBytes);
    }
  return static_cast<char*>(d->row(0)) + bytesPerItem * index;
}

void PiiTypelessMatrix::cloneAndReplaceData(int capacity, std::size_t bytesPerRow)
{
  PiiMatrixData* pData = d->clone(capacity, bytesPerRow);
  d->release();
  d = pData;
}

void PiiTypelessMatrix::reserve(int rows, std::size_t bytesPerRow)
{
  // No more space left -> need to reallocate
  if (rows > d->iCapacity)
    {
      // Many references -> need to copy
      if (d->iRefCount != 1)
        cloneAndReplaceData(rows, bytesPerRow);
      // Only one reference to data -> safe to reallocate
      else
        d = PiiMatrixData::reallocate(d, rows);

      d->iCapacity = rows;
    }
}

void PiiTypelessMatrix::resize(int rows, int columns, std::size_t bytesPerItem)
{
  if (rows == d->iRows && columns == d->iColumns)
    return;

  std::size_t iBytesPerRow = bytesPerItem * columns;
  // No need to change column count
  if (d->bufferType == PiiMatrixData::InternalBuffer &&
      iBytesPerRow <= d->iStride)
    {
      // Reallocate only if capacity is exceeded
      if (rows > d->iCapacity)
        reserve(rows, iBytesPerRow);
      else if (d->iRefCount != 1)
        cloneAndReplaceData(d->iCapacity, iBytesPerRow);

      d->iRows = rows;
      d->iColumns = columns;
    }
  // If we don't own the data or the column count grows too much, need
  // to reallocate.
  else
    {
      PiiMatrixData* pData = PiiMatrixData::createUninitializedData(rows, columns, iBytesPerRow);
      int iMinRows = qMin(rows, d->iRows);
      std::size_t iMinBytes = qMin(iBytesPerRow, bytesPerItem * d->iColumns);
      for (int r=0; r<iMinRows; ++r)
        std::memcpy(pData->row(r), d->row(r), iMinBytes);
      d->release();
      d = pData;
    }
}

void PiiTypelessMatrix::clear()
{
  d->release();
  d = PiiMatrixData::sharedNull();
  d->reserve();
}
