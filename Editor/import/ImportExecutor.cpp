/*
    This file is part of EqualizerAPO-XT.
*/

#include "ImportExecutor.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QTemporaryDir>
#include <QUuid>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace EqAPO::Import
{

namespace
{

bool isReparsePoint(const QString& path)
{
    const DWORD attributes = GetFileAttributesW(
        reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(path).utf16()));
    return attributes != INVALID_FILE_ATTRIBUTES
        && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

bool normalizeRelativeDestination(const QString& relative, QString* normalized)
{
    QString candidate = QDir::cleanPath(QDir::fromNativeSeparators(relative));
    candidate.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (candidate.isEmpty() || candidate == QStringLiteral(".")
        || candidate == QStringLiteral("..") || candidate.startsWith(QStringLiteral("../"))
        || QDir::isAbsolutePath(candidate))
        return false;

    const QStringList parts = candidate.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return false;
    for (const QString& part : parts)
    {
        // Colons would allow a drive-qualified path or NTFS alternate stream;
        // neither belongs in a config-relative import destination.
        if (part == QStringLiteral(".") || part == QStringLiteral("..") || part.contains(QLatin1Char(':')))
            return false;
    }

    *normalized = parts.join(QLatin1Char('/'));
    return true;
}

bool prepareDestination(const QString& configDir, const QString& destRelative,
                        QString* destination, ExecutionResult& result)
{
    QString normalized;
    if (!normalizeRelativeDestination(destRelative, &normalized))
    {
        result.success = false;
        result.errors.append(QObject::tr("Unsafe import destination rejected: %1").arg(destRelative));
        return false;
    }

    if (!QDir().mkpath(configDir))
    {
        result.success = false;
        result.errors.append(QObject::tr("Could not create config directory %1.").arg(configDir));
        return false;
    }

    QFileInfo rootInfo(configDir);
    if (!rootInfo.isDir() || isReparsePoint(rootInfo.absoluteFilePath()))
    {
        result.success = false;
        result.errors.append(QObject::tr("Import target is not a safe directory: %1").arg(configDir));
        return false;
    }

    QString current = QDir::cleanPath(rootInfo.absoluteFilePath());
    const QStringList parts = normalized.split(QLatin1Char('/'));
    for (int i = 0; i + 1 < parts.size(); ++i)
    {
        current = QDir(current).absoluteFilePath(parts.at(i));
        QFileInfo info(current);
        if (info.exists())
        {
            if (!info.isDir() || isReparsePoint(info.absoluteFilePath()))
            {
                result.success = false;
                result.errors.append(QObject::tr("Import destination crosses an unsafe path: %1").arg(current));
                return false;
            }
        }
        else if (!QDir().mkpath(current))
        {
            result.success = false;
            result.errors.append(QObject::tr("Could not create %1.").arg(current));
            return false;
        }
    }

    *destination = QDir::cleanPath(QDir(current).absoluteFilePath(parts.last()));
    return true;
}

bool copyFileItem(const ImportItem& item, const QString& destination, ExecutionResult& result)
{
    QFileInfo sourceInfo(item.sourceAbsolute);
    if (!sourceInfo.exists() || !sourceInfo.isFile() || isReparsePoint(sourceInfo.absoluteFilePath()))
    {
        result.success = false;
        result.errors.append(QObject::tr("Source is not a safe regular file: %1").arg(item.sourceAbsolute));
        return false;
    }

    QFileInfo destinationInfo(destination);
    if (destinationInfo.exists() && destinationInfo.isDir())
    {
        result.success = false;
        result.errors.append(QObject::tr("Could not overwrite directory with file: %1").arg(destination));
        return false;
    }
    if (QFile::exists(destination) && !QFile::remove(destination))
    {
        result.success = false;
        result.errors.append(QObject::tr("Could not overwrite %1.").arg(destination));
        return false;
    }
    if (!QFile::copy(item.sourceAbsolute, destination))
    {
        result.success = false;
        result.errors.append(QObject::tr("Failed to copy %1 to %2.").arg(item.sourceAbsolute, destination));
        return false;
    }

    ++result.filesCopied;
    result.bytesCopied += sourceInfo.size();
    return true;
}

bool copyBundleTree(const QString& sourceRoot, const QString& stagedRoot,
                    int* copiedFiles, qint64* copiedBytes, ExecutionResult& result)
{
    QFileInfo rootInfo(sourceRoot);
    if (!rootInfo.exists() || !rootInfo.isDir() || isReparsePoint(rootInfo.absoluteFilePath()))
    {
        result.success = false;
        result.errors.append(QObject::tr("Source is not a safe VST3 bundle: %1").arg(sourceRoot));
        return false;
    }

    if (!QDir().mkpath(stagedRoot))
    {
        result.success = false;
        result.errors.append(QObject::tr("Could not create staging directory %1.").arg(stagedRoot));
        return false;
    }

    QDir sourceDir(sourceRoot);
    QDirIterator iterator(sourceRoot,
        QDir::Files | QDir::Dirs | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
        QDirIterator::Subdirectories);
    while (iterator.hasNext())
    {
        const QString sourcePath = QDir::cleanPath(iterator.next());
        const QFileInfo sourceInfo(sourcePath);
        if (isReparsePoint(sourceInfo.absoluteFilePath()))
        {
            result.success = false;
            result.errors.append(QObject::tr(
                "VST3 bundle contains a reparse point and was not imported: %1").arg(sourcePath));
            return false;
        }

        QString relative;
        if (!normalizeRelativeDestination(sourceDir.relativeFilePath(sourcePath), &relative))
        {
            result.success = false;
            result.errors.append(QObject::tr("Unsafe VST3 bundle entry rejected: %1").arg(sourcePath));
            return false;
        }
        const QString stagedPath = QDir(stagedRoot).absoluteFilePath(relative);

        if (sourceInfo.isDir())
        {
            if (!QDir().mkpath(stagedPath))
            {
                result.success = false;
                result.errors.append(QObject::tr("Could not create staging directory %1.").arg(stagedPath));
                return false;
            }
            continue;
        }
        if (!sourceInfo.isFile())
        {
            result.success = false;
            result.errors.append(QObject::tr("Unsupported VST3 bundle entry: %1").arg(sourcePath));
            return false;
        }

        if (!QDir().mkpath(QFileInfo(stagedPath).absolutePath()) || !QFile::copy(sourcePath, stagedPath))
        {
            result.success = false;
            result.errors.append(QObject::tr("Failed to stage %1.").arg(sourcePath));
            return false;
        }

        ++*copiedFiles;
        *copiedBytes += sourceInfo.size();
    }

    return true;
}

QString uniqueBackupPath(const QString& destination)
{
    const QFileInfo destinationInfo(destination);
    const QDir parent = destinationInfo.absoluteDir();
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        const QString candidate = parent.absoluteFilePath(
            QStringLiteral(".%1.eapo-import-backup-%2")
                .arg(destinationInfo.fileName(), QUuid::createUuid().toString(QUuid::WithoutBraces)));
        if (!QFileInfo::exists(candidate))
            return candidate;
    }
    return QString();
}

bool copyBundleItem(const ImportItem& item, const QString& configDir,
                    const QString& destination, ExecutionResult& result)
{
    QFileInfo sourceInfo(item.sourceAbsolute);
    if (sourceInfo.suffix().compare(QStringLiteral("vst3"), Qt::CaseInsensitive) != 0)
    {
        result.success = false;
        result.errors.append(QObject::tr("Only VST3 bundle directories can be imported: %1").arg(item.sourceAbsolute));
        return false;
    }

    QFileInfo destinationInfo(destination);
    if (destinationInfo.exists()
        && (!destinationInfo.isDir() || isReparsePoint(destinationInfo.absoluteFilePath())))
    {
        result.success = false;
        result.errors.append(QObject::tr("Could not replace unsafe bundle destination: %1").arg(destination));
        return false;
    }

    QTemporaryDir staging(QDir(configDir).absoluteFilePath(QStringLiteral(".eapo-import-XXXXXX")));
    if (!staging.isValid())
    {
        result.success = false;
        result.errors.append(QObject::tr("Could not create a staging directory in %1.").arg(configDir));
        return false;
    }

    const QString stagedBundle = QDir(staging.path()).absoluteFilePath(destinationInfo.fileName());
    int copiedFiles = 0;
    qint64 copiedBytes = 0;
    if (!copyBundleTree(item.sourceAbsolute, stagedBundle, &copiedFiles, &copiedBytes, result))
        return false;

    QString backup;
    if (destinationInfo.exists())
    {
        backup = uniqueBackupPath(destination);
        if (backup.isEmpty() || !QDir().rename(destination, backup))
        {
            result.success = false;
            result.errors.append(QObject::tr("Could not preserve existing bundle %1.").arg(destination));
            return false;
        }
    }

    if (!QDir().rename(stagedBundle, destination))
    {
        result.success = false;
        result.errors.append(QObject::tr("Could not install staged VST3 bundle at %1.").arg(destination));
        if (!backup.isEmpty() && !QDir().rename(backup, destination))
            result.errors.append(QObject::tr("Could not restore previous bundle from %1.").arg(backup));
        return false;
    }

    if (!backup.isEmpty() && !QDir(backup).removeRecursively())
    {
        result.warnings.append(QObject::tr(
            "Imported bundle successfully, but the previous copy remains at %1.").arg(backup));
    }

    result.filesCopied += copiedFiles;
    result.bytesCopied += copiedBytes;
    return true;
}

}

ExecutionResult ImportExecutor::execute(const ImportManifest& manifest, const QString& configDir)
{
    ExecutionResult result;

    if (configDir.isEmpty())
    {
        result.success = false;
        result.errors.append(QObject::tr("Import target directory is empty."));
        return result;
    }

    for (const ImportItem& item : manifest.items)
    {
        if (!item.exists)
        {
            result.success = false;
            result.errors.append(QObject::tr("Source missing, skipped: %1").arg(item.sourceAbsolute));
            continue;
        }

        QString destination;
        if (!prepareDestination(configDir, item.destRelative, &destination, result))
            continue;

        if (item.payloadKind == ImportPayloadKind::DirectoryTree)
            copyBundleItem(item, configDir, destination, result);
        else
            copyFileItem(item, destination, result);
    }

    return result;
}

}
