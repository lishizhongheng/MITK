/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Language:  C++
Date:      $Date$
Version:   $Revision$

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include <mitkDicomSeriesReader.h>

#include <itkGDCMSeriesFileNames.h>

#if GDCM_MAJOR_VERSION >= 2
  #include <gdcmAttribute.h>
  #include <gdcmPixmapReader.h>
  #include <gdcmStringFilter.h>
  #include <gdcmDirectory.h>
  #include <gdcmScanner.h>
#endif

namespace mitk
{

typedef itk::GDCMSeriesFileNames DcmFileNamesGeneratorType;

DataNode::Pointer 
DicomSeriesReader::LoadDicomSeries(const StringContainer &filenames, bool sort, bool check_4d, UpdateCallBackMethod callback)
{
  DataNode::Pointer node = DataNode::New();

  if (DicomSeriesReader::LoadDicomSeries(filenames, *node, sort, check_4d, callback))
  {
    return node;
  }
  else
  {
    return NULL;
  }
}

bool 
DicomSeriesReader::LoadDicomSeries(const StringContainer &filenames, DataNode &node, bool sort, bool check_4d, UpdateCallBackMethod callback)
{
  if( filenames.empty() )
  {
    MITK_ERROR << "Calling LoadDicomSeries with empty filename string container. Aborting.";
    return false;
  }

  DcmIoType::Pointer io = DcmIoType::New();

  try
  {
    if (io->CanReadFile(filenames.front().c_str()))
    {
      io->SetFileName(filenames.front().c_str());
      io->ReadImageInformation();

      switch (io->GetComponentType())
      {
      case DcmIoType::UCHAR:
        DicomSeriesReader::LoadDicom<unsigned char>(filenames, node, sort, check_4d, callback);
        return true;
      case DcmIoType::CHAR:
        DicomSeriesReader::LoadDicom<char>(filenames, node, sort, check_4d, callback);
        return true;
      case DcmIoType::USHORT:
        DicomSeriesReader::LoadDicom<unsigned short>(filenames, node, sort, check_4d, callback);
        return true;
      case DcmIoType::SHORT:
        DicomSeriesReader::LoadDicom<short>(filenames, node, sort, check_4d, callback);
        return true;
      case DcmIoType::UINT:
        DicomSeriesReader::LoadDicom<unsigned int>(filenames, node, sort, check_4d, callback);
        return true;
      case DcmIoType::INT:
        DicomSeriesReader::LoadDicom<int>(filenames, node, sort, check_4d, callback);
        return true;
      case DcmIoType::ULONG:
        DicomSeriesReader::LoadDicom<long unsigned int>(filenames, node, sort, check_4d, callback);
        return true;
      case DcmIoType::LONG:
        DicomSeriesReader::LoadDicom<long int>(filenames, node, sort, check_4d, callback);
        return true;
      case DcmIoType::FLOAT:
        DicomSeriesReader::LoadDicom<float>(filenames, node, sort, check_4d, callback);
        return true;
      case DcmIoType::DOUBLE:
        DicomSeriesReader::LoadDicom<double>(filenames, node, sort, check_4d, callback);
        return true;
      default:
        MITK_ERROR << "Found unsupported DICOM pixel type: (enum value) " << io->GetComponentType();
      }
    }
  }
  catch(itk::MemoryAllocationError& e)
  {
    MITK_ERROR << "Out of memory. Cannot load DICOM series: " << e.what();
  }
  catch(std::exception& e)
  {
    MITK_ERROR << "Error encountered when loading DICOM series:" << e.what();
  }
  catch(...)
  {
    MITK_ERROR << "Unspecified error encountered when loading DICOM series.";
  }

  return false;
}


bool 
DicomSeriesReader::IsDicom(const std::string &filename)
{
  DcmIoType::Pointer io = DcmIoType::New();

  return io->CanReadFile(filename.c_str());
}

#if GDCM_MAJOR_VERSION >= 2
bool 
DicomSeriesReader::IsPhilips3DDicom(const std::string &filename)
{
  DcmIoType::Pointer io = DcmIoType::New();

  if (io->CanReadFile(filename.c_str()))
  {
    //Look at header Tag 3001,0010 if it is "Philips3D"
    gdcm::Reader reader;
    reader.SetFileName(filename.c_str());
    reader.Read();
    gdcm::DataSet &data_set = reader.GetFile().GetDataSet();
    gdcm::StringFilter sf;
    sf.SetFile(reader.GetFile());

    if (data_set.FindDataElement(gdcm::Tag(0x3001, 0x0010)) &&
      (sf.ToString(gdcm::Tag(0x3001, 0x0010)) == "Philips3D "))
    {
      return true;
    }
  }
  return false;
}

bool 
DicomSeriesReader::ReadPhilips3DDicom(const std::string &filename, mitk::Image::Pointer output_image)
{
  // Now get PhilipsSpecific Tags

  gdcm::PixmapReader reader;
  reader.SetFileName(filename.c_str());
  reader.Read();
  gdcm::DataSet &data_set = reader.GetFile().GetDataSet();
  gdcm::StringFilter sf;
  sf.SetFile(reader.GetFile());

  gdcm::Attribute<0x0028,0x0011> dimTagX; // coloumns || sagittal
  gdcm::Attribute<0x3001,0x1001, gdcm::VR::UL, gdcm::VM::VM1> dimTagZ; //I have no idea what is VM1. // (Philips specific) // transversal
  gdcm::Attribute<0x0028,0x0010> dimTagY; // rows || coronal
  gdcm::Attribute<0x0028,0x0008> dimTagT; // how many frames
  gdcm::Attribute<0x0018,0x602c> spaceTagX; // Spacing in X , unit is "physicalTagx" (usually centimeter)
  gdcm::Attribute<0x0018,0x602e> spaceTagY;
  gdcm::Attribute<0x3001,0x1003, gdcm::VR::FD, gdcm::VM::VM1> spaceTagZ; // (Philips specific)
  gdcm::Attribute<0x0018,0x6024> physicalTagX; // if 3, then spacing params are centimeter
  gdcm::Attribute<0x0018,0x6026> physicalTagY;
  gdcm::Attribute<0x3001,0x1002, gdcm::VR::US, gdcm::VM::VM1> physicalTagZ; // (Philips specific)

  dimTagX.Set(data_set);
  dimTagY.Set(data_set);
  dimTagZ.Set(data_set);
  dimTagT.Set(data_set);
  spaceTagX.Set(data_set);
  spaceTagY.Set(data_set);
  spaceTagZ.Set(data_set);
  physicalTagX.Set(data_set);
  physicalTagY.Set(data_set);
  physicalTagZ.Set(data_set);

  unsigned int
    dimX = dimTagX.GetValue(),
    dimY = dimTagY.GetValue(),
    dimZ = dimTagZ.GetValue(),
    dimT = dimTagT.GetValue(),
    physicalX = physicalTagX.GetValue(),
    physicalY = physicalTagY.GetValue(),
    physicalZ = physicalTagZ.GetValue();

  float
    spaceX = spaceTagX.GetValue(),
    spaceY = spaceTagY.GetValue(),
    spaceZ = spaceTagZ.GetValue();

  if (physicalX == 3) // spacing parameter in cm, have to convert it to mm.
    spaceX = spaceX * 10;

  if (physicalY == 3) // spacing parameter in cm, have to convert it to mm.
    spaceY = spaceY * 10;
  
  if (physicalZ == 3) // spacing parameter in cm, have to convert it to mm.
    spaceZ = spaceZ * 10;

  // Ok, got all necessary Tags!
  // Now read Pixeldata (7fe0,0010) X x Y x Z x T Elements

  const gdcm::Pixmap &pixels = reader.GetPixmap();
  gdcm::RAWCodec codec;

  codec.SetPhotometricInterpretation(gdcm::PhotometricInterpretation::MONOCHROME2);
  codec.SetPixelFormat(pixels.GetPixelFormat());
  codec.SetPlanarConfiguration(0);

  gdcm::DataElement out;
  codec.Decode(data_set.GetDataElement(gdcm::Tag(0x7fe0, 0x0010)), out);

  const gdcm::ByteValue *bv = out.GetByteValue();
  const char *new_pixels = bv->GetPointer();

  // Create MITK Image + Geometry
  typedef itk::Image<unsigned char, 4> ImageType;   //Pixeltype might be different sometimes? Maybe read it out from header
  ImageType::RegionType myRegion;
  ImageType::SizeType mySize;
  ImageType::IndexType myIndex;
  ImageType::SpacingType mySpacing;
  ImageType::Pointer imageItk = ImageType::New();

  mySpacing[0] = spaceX;
  mySpacing[1] = spaceY;
  mySpacing[2] = spaceZ;
  mySpacing[3] = 1;
  myIndex[0] = 0;
  myIndex[1] = 0;
  myIndex[2] = 0;
  myIndex[3] = 0;
  mySize[0] = dimX;
  mySize[1] = dimY;
  mySize[2] = dimZ;
  mySize[3] = dimT;
  myRegion.SetSize( mySize);
  myRegion.SetIndex( myIndex );
  imageItk->SetSpacing(mySpacing);
  imageItk->SetRegions( myRegion);
  imageItk->Allocate();
  imageItk->FillBuffer(0);

  itk::ImageRegionIterator<ImageType>  iterator(imageItk, imageItk->GetLargestPossibleRegion());
  iterator.GoToBegin();
  unsigned long pixCount = 0;
  unsigned long planeSize = dimX*dimY;
  unsigned long planeCount = 0;
  unsigned long timeCount = 0;
  unsigned long numberOfSlices = dimZ;

  while (!iterator.IsAtEnd())
  {
    unsigned long adressedPixel =
      pixCount
      + (numberOfSlices-1-planeCount)*planeSize // add offset to adress the first pixel of current plane
      + timeCount*numberOfSlices*planeSize; // add time offset

    iterator.Set( new_pixels[ adressedPixel ] );
    pixCount++;
    ++iterator;

    if (pixCount == planeSize)
    {
      pixCount = 0;
      planeCount++;
    }
    if (planeCount == numberOfSlices)
    {
      planeCount = 0;
      timeCount++;
    }
    if (timeCount == dimT)
    {
      break;
    }
  }
  mitk::CastToMitkImage(imageItk, output_image);
  return true; // actually never returns false yet.. but exception possible
}
#endif

DicomSeriesReader::UidFileNamesMap 
DicomSeriesReader::GetSeries(const std::string &dir, const StringContainer &restrictions)
{
  UidFileNamesMap map; // result variable

  /**
    assumption about this method:
      returns a map of uid-like-key --> list(filename)
      each entry should contain filenames that have images of same
        - series instance uid (automatically done by GDCMSeriesFileNames
        - 0020,0037 image orientation (patient)
        - 0028,0030 pixel spacing (x,y)
        - 0018,0050 slice thickness
  */


#if GDCM_MAJOR_VERSION < 2
  // old GDCM: let itk::GDCMSeriesFileNames do the sorting

  DcmFileNamesGeneratorType::Pointer name_generator = DcmFileNamesGeneratorType::New();

  name_generator->SetUseSeriesDetails(true);
  name_generator->AddSeriesRestriction("0020|0037"); // image orientation (patient)
  name_generator->AddSeriesRestriction("0028|0030"); // pixel spacing (x,y)

  name_generator->SetLoadSequences(false); // could speed up reading, and we don't use sequences anyway
  name_generator->SetLoadPrivateTags(false);

  for(StringContainer::const_iterator it = restrictions.begin(); 
      it != restrictions.end(); 
      ++it)
  {
    name_generator->AddSeriesRestriction(*it);
  }

  name_generator->SetDirectory(dir.c_str());
  const StringContainer& series_uids = name_generator->GetSeriesUIDs();

  for(StringContainer::const_iterator it = series_uids.begin(); 
      it != series_uids.end(); 
      ++it)
  {
    const std::string& uid = *it;
    map[uid] = name_generator->GetFileNames(uid);
  }

#else
  // new GDCM: use GDCM directly, itk::GDCMSeriesFileNames does not work with GDCM 2

  // set directory
  gdcm::Directory gDirectory;
  gDirectory.Load( dir.c_str(), false ); // non-recursive
  const gdcm::Directory::FilenamesType &gAllFiles = gDirectory.GetFilenames();

  // scann for relevant tags in dicom files
  gdcm::Scanner scanner;
  const gdcm::Tag tagSeriesInstanceUID(0x0020,0x000e); // Series Instance UID
    scanner.AddTag( tagSeriesInstanceUID );

  const gdcm::Tag tagImageOrientation(0x0020, 0x0037); // image orientation
    scanner.AddTag( tagImageOrientation );

  const gdcm::Tag tagPixelSpacing(0x0028, 0x0030); // pixel spacing
    scanner.AddTag( tagPixelSpacing );
    
  const gdcm::Tag tagSliceThickness(0x0018, 0x0050); // slice thickness
    scanner.AddTag( tagSliceThickness );

  const gdcm::Tag tagNumberOfRows(0x0028, 0x0010); // number rows
    scanner.AddTag( tagNumberOfRows );

  const gdcm::Tag tagNumberOfColumns(0x0028, 0x0011); // number cols
    scanner.AddTag( tagNumberOfColumns );

  // TODO add further restrictions from arguments

  // let GDCM scan files
  if ( !scanner.Scan( gDirectory.GetFilenames() ) )
  {
    MITK_ERROR << "gdcm::Scanner failed scanning " << dir;
    return map;
  }

  // assign files IDs that will separate them for loading into image blocks
  for (gdcm::Scanner::ConstIterator fileIter = scanner.Begin();
       fileIter != scanner.End();
       ++fileIter)
  {
    MITK_DEBUG << "File " << fileIter->first << std::endl;
    if ( std::string(fileIter->first).empty() ) continue; // TODO understand why Scanner has empty string entries

    // we const_cast here, because I could not use a map.at() function in CreateMoreUniqueSeriesIdentifier.
    // doing the same thing with find would make the code less readable. Since we forget the Scanner results
    // anyway after this function, we can simply tolerate empty map entries introduced by bad operator[] access
    std::string moreUniqueSeriesId = CreateMoreUniqueSeriesIdentifier( const_cast<gdcm::Scanner::TagToValue&>(fileIter->second) );
    map [ moreUniqueSeriesId ].push_back( fileIter->first );
  }

#endif

  for ( UidFileNamesMap::const_iterator i = map.begin(); i != map.end(); ++i )
  {
    MITK_DEBUG << "Entry " << i->first << " with " << i->second.size() << " files";
  }

  return map;
}

#if GDCM_MAJOR_VERSION >= 2

std::string
DicomSeriesReader::CreateSeriesIdentifierPart( gdcm::Scanner::TagToValue& tagValueMap, const gdcm::Tag& tag )
{
  std::string result;
  try
  {
    result = IDifyTagValue( tagValueMap[ tag ] );
  }
  catch (std::exception& e)
  {
    MITK_ERROR << "Could not access tag " << tag << ": " << e.what();
  }
   
  return result;
}

std::string 
DicomSeriesReader::CreateMoreUniqueSeriesIdentifier( gdcm::Scanner::TagToValue& tagValueMap )
{
  const gdcm::Tag tagSeriesInstanceUID(0x0020,0x000e); // Series Instance UID
  const gdcm::Tag tagImageOrientation(0x0020, 0x0037); // image orientation
  const gdcm::Tag tagPixelSpacing(0x0028, 0x0030); // pixel spacing
  const gdcm::Tag tagSliceThickness(0x0018, 0x0050); // slice thickness
  const gdcm::Tag tagNumberOfRows(0x0028, 0x0010); // number rows
  const gdcm::Tag tagNumberOfColumns(0x0028, 0x0011); // number cols
    
  std::string constructedID;
 
  try
  {
    constructedID = tagValueMap[ tagSeriesInstanceUID ];
  }
  catch (std::exception& e)
  {
    MITK_ERROR << "CreateMoreUniqueSeriesIdentifier() could not access series instance UID. Something is seriously wrong with this image.";
    MITK_ERROR << "Error from exception: " << e.what();
  }
 
  constructedID += CreateSeriesIdentifierPart( tagValueMap, tagNumberOfRows );
  constructedID += CreateSeriesIdentifierPart( tagValueMap, tagNumberOfColumns );
  constructedID += CreateSeriesIdentifierPart( tagValueMap, tagPixelSpacing );
  constructedID += CreateSeriesIdentifierPart( tagValueMap, tagSliceThickness );
  constructedID += CreateSeriesIdentifierPart( tagValueMap, tagImageOrientation );

  constructedID.resize( constructedID.length() - 1 ); // cut of trailing '.'

  MITK_DEBUG << "ID: " << constructedID;
  return constructedID; 
}

std::string 
DicomSeriesReader::IDifyTagValue(const std::string& value)
{
  std::string IDifiedValue( value );
  if (value.empty()) throw std::logic_error("IDifyTagValue() illegaly called with empty tag value");

  // Eliminate non-alnum characters, including whitespace...
  //   that may have been introduced by concats.
  for(std::size_t i=0; i<IDifiedValue.size(); i++)
  {
    while(i<IDifiedValue.size() 
      && !( IDifiedValue[i] == '.'
        || (IDifiedValue[i] >= 'a' && IDifiedValue[i] <= 'z')
        || (IDifiedValue[i] >= '0' && IDifiedValue[i] <= '9')
        || (IDifiedValue[i] >= 'A' && IDifiedValue[i] <= 'Z')))
    {
      IDifiedValue.erase(i, 1);
    }
  }


  IDifiedValue += ".";
  return IDifiedValue;
}
#endif

DicomSeriesReader::StringContainer 
DicomSeriesReader::GetSeries(const std::string &dir, const std::string &series_uid, const StringContainer &restrictions)
{
  UidFileNamesMap allSeries = GetSeries(dir, restrictions);
  StringContainer resultingFileList;

  for ( UidFileNamesMap::const_iterator idIter = allSeries.begin(); 
        idIter != allSeries.end(); 
        ++idIter )
  {
    if ( idIter->first.find( series_uid ) == 0 ) // this ID starts with given series_uid
    {
      resultingFileList.insert( resultingFileList.end(), idIter->second.begin(), idIter->second.end() ); // append
    }
  }

  return resultingFileList;
}

DicomSeriesReader::StringContainer 
DicomSeriesReader::SortSeriesSlices(const StringContainer &unsortedFilenames)
{
#if GDCM_MAJOR_VERSION >= 2
  gdcm::Sorter sorter;

  sorter.SetSortFunction(DicomSeriesReader::GdcmSortFunction);
  sorter.Sort(unsortedFilenames);
  return sorter.GetFilenames();
#else
  return unsortedFilenames;
#endif
}

#if GDCM_MAJOR_VERSION >= 2
bool 
DicomSeriesReader::GdcmSortFunction(const gdcm::DataSet &ds1, const gdcm::DataSet &ds2)
{
  gdcm::Attribute<0x0020,0x0032> image_pos1; // Image Position (Patient)
  gdcm::Attribute<0x0020,0x0037> image_orientation1; // Image Orientation (Patient)

  image_pos1.Set(ds1);
  image_orientation1.Set(ds1);

  gdcm::Attribute<0x0020,0x0032> image_pos2;
  gdcm::Attribute<0x0020,0x0037> image_orientation2;

  image_pos2.Set(ds2);
  image_orientation2.Set(ds2);

  if (image_orientation1 != image_orientation2)
  {
    MITK_ERROR << "Dicom images have different orientations.";
    throw std::logic_error("Dicom images have different orientations. Call GetSeries() first to separate images.");
  }

  double normal[3];

  normal[0] = image_orientation1[1] * image_orientation1[5] - image_orientation1[2] * image_orientation1[4];
  normal[1] = image_orientation1[2] * image_orientation1[3] - image_orientation1[0] * image_orientation1[5];
  normal[2] = image_orientation1[0] * image_orientation1[4] - image_orientation1[1] * image_orientation1[3];

  double
      dist1 = 0.0,
      dist2 = 0.0;

  for (unsigned char i = 0u; i < 3u; ++i)
  {
    dist1 += normal[i] * image_pos1[i];
    dist2 += normal[i] * image_pos2[i];
  }

  if ( fabs(dist1 - dist2) < mitk::eps)
  {
    gdcm::Attribute<0x0008,0x0032> acq_time1; // Acquisition time (may be missing, so we check existence first)
    gdcm::Attribute<0x0008,0x0032> acq_time2;

    if (ds1.FindDataElement(gdcm::Tag(0x0008,0x0032)))
      acq_time1.Set(ds1);

    if (ds2.FindDataElement(gdcm::Tag(0x0008,0x0032)))
      acq_time2.Set(ds2);

    // exception: same position: compare by acquisition time
    return acq_time1 < acq_time2;
  }
  else
  {
    // default: compare position
    return dist1 < dist2;
  }
}
#endif
  
std::string DicomSeriesReader::GetConfigurationString()
{
  std::stringstream configuration;
  configuration << "MITK_USE_GDCMIO: ";
#ifdef MITK_USE_GDCMIO
  configuration << "true";
#else
  configuration << "false";
#endif
  configuration << "\n";

  configuration << "GDCM_VERSION: ";
#ifdef GDCM_MAJOR_VERSION
  configuration << GDCM_VERSION;
#endif
  //configuration << "\n";

  return configuration.str();
}

}

#include <mitkDicomSeriesReader.txx>

