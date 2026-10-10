/*
	This file is part of EqualizerAPO-XT, a system-wide equalizer.
	Copyright (C) 2026 115dkk
	SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "EditorLogicTestSupport.h"

#include <string>
#include <thread>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "Editor/ConfigFileCodec.h"
#include "platform/windows/Win32Resource.h"

void testConfigFileCodec()
{
	QList<QString> mixed = ConfigFileCodec::decodeLines(std::string("Preamp: -6 dB\r\nInclude: a.txt\nlast"));
	requireEqual((int)mixed.size(), 3, "decodeLines splits CRLF and LF terminated lines");
	expectEqual(mixed[0], "Preamp: -6 dB", "decodeLines strips the trailing CR");
	expectEqual(mixed[2], "last", "decodeLines keeps the final unterminated line");

	QList<QString> unicode = ConfigFileCodec::decodeLines(std::string("# caf\xC3\xA9"));
	expectEqual(unicode[0], QString::fromUtf8("# caf\xC3\xA9"), "decodeLines decodes valid UTF-8");

	// 0xE9 alone is invalid UTF-8, so the system-ANSI fallback must engage.
	// The decoded glyph depends on the machine's CP_ACP (e.g. CP1252 vs
	// CP949), so only the line structure is asserted, not the character.
	QList<QString> fallback = ConfigFileCodec::decodeLines(std::string("caf\xE9\r\nnext"));
	requireEqual((int)fallback.size(), 2, "ANSI fallback still yields one entry per line");
	expectEqual(fallback[1], "next", "ANSI fallback preserves the following line");

	QByteArray encoded = ConfigFileCodec::encodeLines(QList<QString>() << "a" << QString::fromUtf8("caf\xC3\xA9"));
	expectEqual(QString::fromUtf8(encoded), QString::fromUtf8("a\r\ncaf\xC3\xA9"), "encodeLines joins with CRLF, UTF-8, no trailing newline");

	QList<QString> roundTrip = ConfigFileCodec::decodeLines(std::string(encoded.constData(), (size_t)encoded.size()));
	requireEqual((int)roundTrip.size(), 2, "encodeLines output decodes back to the same line count");
	expectEqual(roundTrip[1], QString::fromUtf8("caf\xC3\xA9"), "decode(encode(lines)) round-trips non-ASCII text");
}

void testConfigFileCodecRetriesAtomicReplaceAfterReaderCloses()
{
	QTemporaryDir dir;
	requireTrue(dir.isValid(), "atomic-save retry test creates a temporary directory");

	QString path = QDir::toNativeSeparators(dir.filePath("config.txt"));
	QFile original(path);
	requireTrue(original.open(QIODevice::WriteOnly), "atomic-save retry test creates the original file");
	requireTrue(original.write("original") == 8, "atomic-save retry test writes the original contents");
	original.close();

	winutil::UniqueHandle reader(CreateFileW(
		path.toStdWString().c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr));
	requireTrue(static_cast<bool>(reader), "atomic-save retry test holds the engine-style read handle");

	std::thread releaseReader([reader = std::move(reader)]() mutable {
		Sleep(20);
		reader.reset();
	});
	ConfigFileCodec::WriteResult result = ConfigFileCodec::writeConfig(
		path, QList<QString>() << "replacement");
	releaseReader.join();

	expectTrue(result.opened, "writeConfig retries until a short configuration read finishes");

	QFile committed(path);
	requireTrue(committed.open(QIODevice::ReadOnly), "atomic-save retry test opens the committed file");
	expectEqual(QString::fromUtf8(committed.readAll()), "replacement",
		"the retry atomically commits the complete replacement");
}

void testConfigFileCodecRetriesAtomicReplaceAfterSlowReaderCloses()
{
	QTemporaryDir dir;
	requireTrue(dir.isValid(), "slow atomic-save retry test creates a temporary directory");

	QString path = QDir::toNativeSeparators(dir.filePath("config.txt"));
	QFile original(path);
	requireTrue(original.open(QIODevice::WriteOnly), "slow atomic-save retry test creates the original file");
	requireTrue(original.write("original") == 8, "slow atomic-save retry test writes the original contents");
	original.close();

	// The audio engine can still be parsing an included configuration after the
	// editor begins an atomic replacement.  This is the same no-delete reader
	// used by the engine-side path policy, but it lives long enough to exceed
	// the old short editor retry budget.
	winutil::UniqueHandle reader(CreateFileW(
		path.toStdWString().c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr));
	requireTrue(static_cast<bool>(reader), "slow atomic-save retry test holds the engine-style read handle");

	std::thread releaseReader([reader = std::move(reader)]() mutable {
		Sleep(750);
		reader.reset();
	});
	ConfigFileCodec::WriteResult result = ConfigFileCodec::writeConfig(
		path, QList<QString>() << "replacement");
	releaseReader.join();

	expectTrue(result.opened, "writeConfig waits for a bounded engine-side configuration read to finish");

	QFile committed(path);
	requireTrue(committed.open(QIODevice::ReadOnly), "slow atomic-save retry test opens the committed file");
	expectEqual(QString::fromUtf8(committed.readAll()), "replacement",
		"the delayed retry atomically commits the complete replacement");
}

void testConfigFileCodecPreservesExistingFileWhenAtomicReplaceFails()
{
	QTemporaryDir dir;
	requireTrue(dir.isValid(), "atomic-save test creates a temporary directory");

	QString path = QDir::toNativeSeparators(dir.filePath("config.txt"));
	QFile original(path);
	requireTrue(original.open(QIODevice::WriteOnly), "atomic-save test creates the original file");
	requireTrue(original.write("original") == 8, "atomic-save test writes the original contents");
	original.close();

	// Access 0 with read/write sharing still permits in-place writes, while
	// omitting FILE_SHARE_DELETE prevents replacement while this handle lives.
	winutil::UniqueHandle replacementBlocker(CreateFileW(
		path.toStdWString().c_str(),
		0,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr));
	requireTrue(static_cast<bool>(replacementBlocker), "atomic-save test locks replacement of the original file");

	ConfigFileCodec::WriteResult result = ConfigFileCodec::writeConfig(path, QList<QString>() << "replacement");
	replacementBlocker.reset();

	expectFalse(result.opened, "writeConfig reports an atomic replacement failure");

	QFile preserved(path);
	requireTrue(preserved.open(QIODevice::ReadOnly), "atomic-save test reopens the original file");
	expectEqual(QString::fromUtf8(preserved.readAll()), "original", "failed atomic replacement preserves the original contents");
}

void testConfigFileCodecAtomicByteRestore()
{
	QTemporaryDir dir;
	requireTrue(dir.isValid(), "byte restore creates an isolated directory");
	const QString path = QDir::toNativeSeparators(dir.filePath("snapshot.txt"));
	// A snapshot may contain an ANSI byte, mixed line endings and a BOM. None
	// may be normalized when restoring an existing user's file.
	const QByteArray bytes = QByteArray::fromHex("efbbbf23e90a6e6578740d0a");
	const ConfigFileCodec::WriteResult initial = ConfigFileCodec::writeBytes(path, bytes);
	requireTrue(initial.opened, "the byte writer creates a complete atomic snapshot");
	QFile contents(path);
	requireTrue(contents.open(QIODevice::ReadOnly), "the byte restore opens its snapshot");
	expectTrue(contents.readAll() == bytes, "byte restore preserves encoding and line endings exactly");
	contents.close();

	winutil::UniqueHandle reader(CreateFileW(path.toStdWString().c_str(), GENERIC_READ,
		FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
	requireTrue(static_cast<bool>(reader), "byte restore holds a temporary no-delete reader");
	std::thread releaseReader([reader = std::move(reader)]() mutable {
		Sleep(750);
		reader.reset();
	});
	const QByteArray next = bytes + QByteArray("tail\n");
	const ConfigFileCodec::WriteResult restored = ConfigFileCodec::writeBytes(path, next);
	releaseReader.join();
	expectTrue(restored.opened, "byte restore waits for a bounded reader to close");
	requireTrue(contents.open(QIODevice::ReadOnly), "byte restore reopens the committed snapshot");
	expectTrue(contents.readAll() == next, "the delayed byte restore remains byte-exact");
	contents.close();

	winutil::UniqueHandle blocker(CreateFileW(path.toStdWString().c_str(), 0,
		FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
	requireTrue(static_cast<bool>(blocker), "byte restore blocks replacement while permitting in-place writes");
	const ConfigFileCodec::WriteResult denied = ConfigFileCodec::writeBytes(path, bytes);
	blocker.reset();
	expectFalse(denied.opened, "byte restore refuses a persistent replacement lock");
	requireTrue(contents.open(QIODevice::ReadOnly), "byte restore reads the preserved target after failure");
	expectTrue(contents.readAll() == next, "failed byte restore never truncates the target");
}

void testConfigFileCodecRejectsPartialRead()
{
	const std::wstring pipeName = L"\\\\.\\pipe\\EditorLogicTests-ConfigRead-" + std::to_wstring(GetCurrentProcessId());
	winutil::UniqueHandle pipe(CreateNamedPipeW(
		pipeName.c_str(),
		PIPE_ACCESS_OUTBOUND,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1,
		4096,
		4096,
		0,
		nullptr));
	requireTrue(static_cast<bool>(pipe), "partial-read test creates a named pipe");

	bool serverSucceeded = false;
	std::thread server([&]() {
		BOOL connected = ConnectNamedPipe(pipe.get(), nullptr);
		if (!connected && GetLastError() == ERROR_PIPE_CONNECTED)
			connected = TRUE;
		const char prefix[] = "Preamp: -6 dB\r\n";
		DWORD written = 0;
		if (connected && WriteFile(pipe.get(), prefix, sizeof(prefix) - 1, &written, nullptr) && written == sizeof(prefix) - 1)
			serverSucceeded = FlushFileBuffers(pipe.get()) != FALSE;
		DisconnectNamedPipe(pipe.get());
	});

	ConfigFileCodec::ReadResult result = ConfigFileCodec::readConfig(QString::fromStdWString(pipeName));
	server.join();
	requireTrue(serverSucceeded, "partial-read test sends the configuration prefix");
	expectFalse(result.ok, "readConfig rejects bytes followed by a ReadFile failure");
	expectTrue(result.lines.isEmpty(), "readConfig does not expose a partial configuration");
}
