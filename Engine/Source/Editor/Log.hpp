#pragma once

namespace helios::editor
{
	enum class LogMessageTypes : uint8_t
	{
		Info,
		Warn,
		Error
	};

	void LogMessage(std::wstring_view message, const LogMessageTypes &messageType);
}