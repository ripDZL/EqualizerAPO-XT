#pragma once

#include <QObject>
#include <QString>

#include "ReferenceCardView.h"

class QWidget;

class FileReferenceController : public QObject
{
public:
	FileReferenceController(const QString& kind, const QString& writtenPath,
		QObject* parent = nullptr);

	const QString& writtenPath() const;
	const QString& resolvedPath() const;
	void setWrittenPath(const QString& path);
	void setResolvedPath(const QString& path);
	bool setVST3BundleSelection(const QString& selectedPath,
		const QString& referenceBaseDirectory);
	void resolveAgainstConfig(const QString& configPath);
	void resolveAgainstDirectory(const QString& directoryPath);

	// selectVst3Bundles additionally lets the dialog pick *.vst3 bundle
	// directories as if they were files (GUIHelper::enableVst3BundleSelection).
	QString chooseExistingFile(QWidget* parent, const QString& title,
		const QString& initialPath, const QString& nameFilter,
		const QString& referenceBaseDirectory,
        const QString& selectedFile = QString(),
        bool selectVst3Bundles = false);
    QString chooseExistingVST3Bundle(QWidget* parent, const QString& title,
        const QString& initialPath, const QString& referenceBaseDirectory,
        const QString& selectedDirectory = QString(), bool* invalidSelection = nullptr);
	ReferenceCardState describe(const QString& emptyName) const;
	static bool isReadableByAudioService(const QString& absolutePath);
	static bool isVST3BundleDirectory(const QString& absolutePath);
	bool importIntoConfig(QWidget* parent, const QString& configPath);

	static QString displayPathForBaseDirectory(
		const QString& baseDirectory, const QString& selectedPath);

private:
	QString referenceKind;
	QString written;
	QString resolved;
};
