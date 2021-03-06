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

#include "PiiVideoFileWriter.h"
#include <PiiYdinTypes.h>
#include <PiiColor.h>

PiiVideoFileWriter::Data::Data() :
  strOutputDirectory("."), strFileName("output.mpg"), iIndex(0),
  iWidth(0), iHeight(0), iFrameRate(25), pVideoWriter(0)
{}

PiiVideoFileWriter::PiiVideoFileWriter() :
  PiiDefaultOperation(new Data)
{
  setThreadCount(1);
  PII_D;
  d->pImageInput = new PiiInputSocket("image");
  addSocket(d->pImageInput);

  connect(this, SIGNAL(stateChanged(PiiOperation::State)),
          SLOT(deletePiiVideoWriter(PiiOperation::State)),
          Qt::DirectConnection);
}

PiiVideoFileWriter::~PiiVideoFileWriter()
{
  PII_D;
  delete d->pVideoWriter;
}

void PiiVideoFileWriter::process()
{
  PII_D;
  using namespace PiiYdin;
  PiiVariant obj = d->pImageInput->firstObject();

  if (d->iIndex == 0 )
    {
      switch (obj.type())
        {
          PII_INTEGER_MATRIX_CASES(initPiiVideoWriter, obj);
          PII_UNSIGNED_MATRIX_CASES(initPiiVideoWriter, obj);
          PII_FLOAT_MATRIX_CASES(initPiiVideoWriter, obj);
        case UnsignedCharColorMatrixType:
          initPiiVideoWriter<PiiColor<unsigned char> >(obj);
          break;
        case UnsignedCharColor4MatrixType:
          initPiiVideoWriter<PiiColor4<unsigned char> >(obj);
          break;
        default:
          PII_THROW_UNKNOWN_TYPE(d->pImageInput);
        }
    }

  if (d->pVideoWriter != 0 )
    {
      switch (obj.type())
        {
          PII_INTEGER_MATRIX_CASES(grayImage, obj);
          PII_UNSIGNED_MATRIX_CASES(grayImage, obj);
          PII_FLOAT_MATRIX_CASES(floatImage, obj);
        case UnsignedCharColorMatrixType:
          colorImage<PiiColor<unsigned char> >(obj);
          break;
        case UnsignedCharColor4MatrixType:
          colorImage<PiiColor4<unsigned char> >(obj);
          break;
        default:
          PII_THROW_UNKNOWN_TYPE(d->pImageInput);
        }
    }

  ++d->iIndex;

}

void PiiVideoFileWriter::deletePiiVideoWriter(PiiOperation::State state)
{
  PII_D;

  if ( state == PiiOperation::Stopped )
    {
      delete d->pVideoWriter;
      d->pVideoWriter = 0;
      d->iIndex = 0;
    }
}

template <class T> void PiiVideoFileWriter::initPiiVideoWriter(const PiiVariant& obj)
{
  PII_D;

  const PiiMatrix<T> matrix = obj.valueAs<PiiMatrix<T> >();

  if (d->strFileName.isEmpty() )
    {
      PII_THROW(PiiExecutionException, tr("Video writer cannot start because video file name have "
                                          "not been set or file name is empty."));
    }

  QString filename = QString("%1/%2").arg(d->strOutputDirectory).arg(d->strFileName);

   d->iWidth = matrix.columns();
   d->iHeight = matrix.rows();

   if ( d->pVideoWriter == 0 )
     d->pVideoWriter = new PiiVideoWriter(filename, d->iWidth, d->iHeight, d->iFrameRate);
   else
     {
       d->pVideoWriter->setFileName(filename);
       d->pVideoWriter->setWidth(d->iWidth);
       d->pVideoWriter->setHeight(d->iHeight);
       d->pVideoWriter->setFrameRate(d->iFrameRate);
     }

   try
     {
       d->pVideoWriter->initialize();
     }
   catch(PiiVideoException& ob)
     {
       QString message = tr("Video writer cannot start reason: ");
       message +=ob.message();
       PII_THROW(PiiExecutionException, message.toStdString().c_str());
     }

}

template <class T> void PiiVideoFileWriter::grayImage( const PiiVariant& obj )
{
  PII_D;
  const PiiMatrix<T> mat = obj.valueAs<PiiMatrix<T> >();
  bool bSave = false;
  if ( mat.columns() == d->iWidth || mat.rows() == d->iHeight )
    bSave = d->pVideoWriter->saveNextGrayFrame(PiiMatrix<unsigned char>(mat));
  else
    {
      PII_THROW(PiiExecutionException, tr("Input frame might be corrupted."));
    }
  if(!bSave)
    {
      PII_THROW(PiiExecutionException, tr("Input frame was not saved correctly"));
    }
}

template <class T> void PiiVideoFileWriter::floatImage( const PiiVariant& obj )
{
  PII_D;
  const PiiMatrix<T> mat = obj.valueAs<PiiMatrix<T> >();
  bool bSave = false;
  if ( mat.columns() == d->iWidth || mat.rows() == d->iHeight )
    d->pVideoWriter->saveNextGrayFrame(PiiMatrix<unsigned char>(mat * 255));
  else
    {
      PII_THROW(PiiExecutionException, tr("Input frame might be corrupted."));
    }
   if(!bSave)
    {
      PII_THROW(PiiExecutionException, tr("Input frame was not saved correctly"));
    }
}

template <class T> void PiiVideoFileWriter::colorImage( const PiiVariant& obj )
{
  PII_D;
  const PiiMatrix<T> mat = obj.valueAs<PiiMatrix<T> >();
  bool bSave = false;
  if ( mat.columns() == d->iWidth || mat.rows() == d->iHeight )
    d->pVideoWriter->saveNextColorFrame(PiiMatrix<PiiColor<unsigned char> >(mat));
  else
    {
      PII_THROW(PiiExecutionException, tr("Input frame might be corrupted."));
    }
  if(!bSave)
    {
      PII_THROW(PiiExecutionException, tr("Input frame was not saved correctly"));
    }

}

QString PiiVideoFileWriter::outputDirectory() const { return _d()->strOutputDirectory; }
void PiiVideoFileWriter::setOutputDirectory(const QString& dirName) { _d()->strOutputDirectory = dirName; }
QString PiiVideoFileWriter::fileName() const { return _d()->strFileName; }
void PiiVideoFileWriter::setFileName(const QString& fileName) { _d()->strFileName = fileName; }
int PiiVideoFileWriter::frameRate() const { return _d()->iFrameRate; }
void PiiVideoFileWriter::setFrameRate(int frameRate) { _d()->iFrameRate = frameRate; }
