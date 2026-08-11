#include "EditorLogicTestSupport.h"

#include <new>
#include <stdexcept>

#include <QDir>
#include <QFile>
#include <QLocale>
#include <QTemporaryDir>

#include "Benchmark/BatchPlan.h"
#include "Editor/helpers/AnalysisWorkerRecovery.h"
#include "Editor/widgets/EditableValueText.h"
#include "Editor/widgets/cards/FileReferenceController.h"
#include "helpers/MemoryHelper.h"

void testEditableValueTextUsesDisplayedDecimalFormatFirst()
{
	const QLocale german(QLocale::German, QLocale::Germany);
	double value = 0.0;
	expectTrue(parseEditableValueText(QStringLiteral("12.345"), german, &value),
		"dot-decimal text parses under a grouping-dot locale");
	expectTrue(qAbs(value - 12.345) < 0.000001,
		"dot-decimal text keeps the displayed C-locale meaning");
	expectTrue(parseEditableValueText(QStringLiteral("12,5"), german, &value),
		"system-locale decimal text remains accepted as a fallback");
	expectTrue(qAbs(value - 12.5) < 0.000001,
		"system-locale fallback keeps its decimal meaning");
}

void testBenchmarkBatchPlanUsesOnlyComparableFullBatches()
{
	const BenchmarkBatchPlan partial = planBenchmarkBatches(1000, 480);
	expectEqual(partial.processedFrames, 960u,
		"benchmark processes only full fixed-size batches");
	expectEqual(partial.trimmedFrames, 40u,
		"benchmark reports the excluded partial tail");

	const BenchmarkBatchPlan exact = planBenchmarkBatches(960, 480);
	expectEqual(exact.processedFrames, 960u,
		"batch-aligned benchmark lengths remain unchanged");
	expectEqual(exact.trimmedFrames, 0u,
		"batch-aligned benchmark lengths trim nothing");
}

void testFileReferenceControllerOwnsPathState()
{
	QTemporaryDir temp;
	requireTrue(temp.isValid(), "file-reference test creates a temporary root");
	QDir root(temp.path());
	requireTrue(root.mkpath(QStringLiteral("config/irs")),
		"file-reference test creates the config dependency directory");
	requireTrue(root.mkpath(QStringLiteral("outside")),
		"file-reference test creates the external directory");
	const QString configPath = root.filePath(QStringLiteral("config/config.txt"));
	const QString impulsePath = root.filePath(QStringLiteral("config/irs/room.wav"));
	QFile impulse(impulsePath);
	requireTrue(impulse.open(QIODevice::WriteOnly),
		"file-reference test creates the resolved dependency");
	impulse.close();

	FileReferenceController reference(
		QStringLiteral("convolution"), QStringLiteral("irs/room.wav"));
	reference.resolveAgainstConfig(configPath);
	ReferenceCardState state = reference.describe(QStringLiteral("No file selected"));
	expectFalse(state.missing, "controller resolves a config-relative reference");
	expectEqual(state.name, QStringLiteral("room.wav"),
		"controller derives the reference display name");
	expectEqual(state.directory, QDir::toNativeSeparators(QStringLiteral("irs")),
		"controller preserves the as-written relative directory");
	expectPath(state.fullPath, impulsePath);

	reference.setWrittenPath(QStringLiteral("irs/missing.wav"));
	reference.resolveAgainstConfig(configPath);
	state = reference.describe(QStringLiteral("No file selected"));
	expectTrue(state.missing, "controller owns and reports an edited missing path");
	expectTrue(state.fullPath.isEmpty(), "missing references do not expose a clickable path");

	const QString baseDirectory = root.filePath(QStringLiteral("config/deep"));
	const QString farSelection = root.filePath(QStringLiteral("outside/plugin.dll"));
	expectPath(FileReferenceController::displayPathForBaseDirectory(
		baseDirectory, farSelection), farSelection);
}

void testAnalysisWorkerRecovery()
{
	bool failurePublished = false;
	try
	{
		const bool succeeded = AnalysisWorkerRecovery::run(
			[] { throw std::bad_alloc(); },
			[&](const char*) { failurePublished = true; });
		expectFalse(succeeded, "failed analysis iteration reported success");
	}
	catch (...)
	{
		harness.fail("analysis iteration exception escaped and would stop the worker");
	}
	expectTrue(failurePublished, "analysis failure was not published");

	bool nextIterationRan = false;
	const bool recovered = AnalysisWorkerRecovery::run(
		[&] { nextIterationRan = true; },
		[](const char*) {});
	expectTrue(recovered, "next analysis iteration did not recover");
	expectTrue(nextIterationRan, "analysis worker did not accept work after a failure");
}

void testMemoryHelperConstructReleasesStorageWhenConstructorThrows()
{
	struct ThrowingConstructor
	{
		ThrowingConstructor()
		{
			throw std::runtime_error("constructor failure");
		}
	};

	// The counters live in MemoryHelper itself; this binary links Common.lib
	// whole-archive and therefore cannot substitute its own alloc()/free().
	MemoryHelper::resetAllocationCountsForTesting();
	bool threw = false;
	try
	{
		MemoryHelper::construct<ThrowingConstructor>();
	}
	catch (const std::runtime_error&)
	{
		threw = true;
	}

	expectTrue(threw, "construct propagates a constructor exception");
	expectEqual((int)MemoryHelper::allocationCountForTesting(), 1, "construct allocates storage once");
	expectEqual((int)MemoryHelper::freeCountForTesting(), 1, "construct releases storage when construction fails");
}
