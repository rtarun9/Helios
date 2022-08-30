#pragma once

namespace helios::core
{
	enum class LogMessageTypes : int
	{
		Info,
		Warn,
		Error
	};

	struct ApplicationLog
	{
		static void AddLog(std::string_view logMessage, const core::LogMessageTypes& messageType);

		static inline std::vector<std::string> textBuffer;
		static inline std::vector<core::LogMessageTypes> messageTypes;
	};

	void LogMessage(std::wstring_view message, const LogMessageTypes &messageType);
}