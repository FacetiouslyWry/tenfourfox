/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "GMPUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsCOMPtr.h"
#include "nsLiteralString.h"
#include "nsCRTGlue.h"
#include "mozilla/Base64.h"
#include "nsISimpleEnumerator.h"

namespace mozilla {

bool
GetEMEVoucherPath(nsIFile** aPath)
{
  nsCOMPtr<nsIFile> path;
  NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(path));
  if (!path) {
    NS_WARNING("GetEMEVoucherPath can't get NS_GRE_DIR!");
    return false;
  }
  path->AppendNative(NS_LITERAL_CSTRING("voucher.bin"));
  path.forget(aPath);
  return true;
}

bool
EMEVoucherFileExists()
{
  nsCOMPtr<nsIFile> path;
  bool exists;
  return GetEMEVoucherPath(getter_AddRefs(path)) &&
         NS_SUCCEEDED(path->Exists(&exists)) &&
         exists;
}

void
SplitAt(const char* aDelims,
        const nsACString& aInput,
        nsTArray<nsCString>& aOutTokens)
{
  nsAutoCString str(aInput);
  char* end = str.BeginWriting();
  const char* start = nullptr;
  while (!!(start = NS_strtok(aDelims, &end))) {
    aOutTokens.AppendElement(nsCString(start));
  }
}

nsCString
ToBase64(const nsTArray<uint8_t>& aBytes)
{
  nsAutoCString base64;
  nsDependentCSubstring raw(reinterpret_cast<const char*>(aBytes.Elements()),
                            aBytes.Length());
  nsresult rv = Base64Encode(raw, base64);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return NS_LITERAL_CSTRING("[Base64EncodeFailed]");
  }
  return base64;
}

bool
FileExists(nsIFile* aFile)
{
  bool exists = false;
  return aFile && NS_SUCCEEDED(aFile->Exists(&exists)) && exists;
}

DirectoryEnumerator::DirectoryEnumerator(nsIFile* aPath, Mode aMode)
  : mMode(aMode)
{
  aPath->GetDirectoryEntries(getter_AddRefs(mIter));
}

already_AddRefed<nsIFile>
DirectoryEnumerator::Next()
{
  if (!mIter) {
    return nullptr;
  }
  bool hasMore = false;
  while (NS_SUCCEEDED(mIter->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    nsresult rv = mIter->GetNext(getter_AddRefs(supports));
    if (NS_FAILED(rv)) {
      continue;
    }

    nsCOMPtr<nsIFile> path(do_QueryInterface(supports, &rv));
    if (NS_FAILED(rv)) {
      continue;
    }

    if (mMode == DirsOnly) {
      bool isDirectory = false;
      rv = path->IsDirectory(&isDirectory);
      if (NS_FAILED(rv) || !isDirectory) {
        continue;
      }
    }
    return path.forget();
  }
  return nullptr;
}

} // namespace mozilla