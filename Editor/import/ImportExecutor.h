/*
    This file is part of EqualizerAPO-XT.

    Side-effecting half of the "import to config directory" flow.
    Takes an ImportManifest produced by ConfigDependencyScanner and
    copies every existing item into the config directory, creating
    sub-directories as needed and overwriting any existing destination.
*/

#pragma once

#include "ImportManifest.h"

#include <QString>
#include <QStringList>

#include <cstdint>

namespace EqAPO::Import
{

struct ExecutionResult
{
    bool success = true;
    int filesCopied = 0;
    qint64 bytesCopied = 0;
    QStringList errors;
    // The import itself completed, but a recoverable cleanup task (such as
    // deleting an old replaced bundle) needs the user's attention.
    QStringList warnings;
};

class ImportExecutor
{
public:
    // Apply the manifest to configDir. File items retain the existing
    // overwrite behavior; DirectoryTree items are staged, validated against
    // reparse points, then atomically swapped into place. Returns success
    // only if every item reached its destination.
    static ExecutionResult execute(const ImportManifest& manifest, const QString& configDir);
};

}
