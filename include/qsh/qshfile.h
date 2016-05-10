
#ifndef QSHFILE_H
#define QSHFILE_H

#include <istream>
#include <memory>
#include <vector>

#include "qsh/types.h"

namespace qsh
{
	template <typename Sink>
	class QshFile
	{
	public:

		static const int SupportedVersion = 4;

		enum class StreamType
		{
			Quotes = 0x10,
			Deals = 0x20,
			OwnOrders = 0x30,
			OwnDeals = 0x40,
			Messages = 0x50,
			AuxInfo = 0x60,
			OrdLog = 0x70
		};

		struct StreamId
		{
			StreamType type;
			std::string connector;
			std::string ticker;
			std::string auxcode;
			int numId;
			int step;
		};

		struct DepthItem
		{
			decimal_fixed value;
			int volume;
		};

		struct Tick
		{
			datetime_t timestamp;
			long tradeId;
			long orderId;
			int volume;
			long openInterest;
		};

		struct OrderLogEntry
		{
			enum Flags
			{
				NonZeroReplAct = (1 << 0),
				SessIdChanged = (1 << 1),
				Add = (1 << 2),
				Fill = (1 << 3),
				Buy = (1 << 4),
				Sell = (1 << 5),
				Quote = (1 << 7),
				Counter = (1 << 8),
				NonSystem = (1 << 9),
				EndOfTransaction = (1 << 10),
				FillOrKill = (1 << 11),
				Moved = (1 << 12),
				Cancelled = (1 << 13),
				CancelledGroup = (1 << 14),
				CrossTrade = (1 << 15)
			};

			int streamNumber;
			uint16_t flags;
			datetime_t timestamp;
			long long orderId;
			decimal_fixed orderPrice;
			int volume;
			int remain;
			long long matchingOrderId;
			decimal_fixed tradePrice;
			long openInterest;
		};

		struct Metadata
		{
			std::string applicationName;
			std::string comment;
			datetime_t startTime;
			int streamsNumber;
		};

		QshFile(std::istream& stream, Sink& sink) : stream_(stream),
			sink_(sink)
		{
			if(!stream_.good())
				throw std::runtime_error("Unable to open stream");

			readMetadata();
		}

		~QshFile()
		{
		}

		std::vector<StreamId> streams() const
		{
			std::vector<StreamId> result;
			for(const auto& stream : streams_)
			{
				result.push_back(stream.id);
			}
			return result;
		}

		Metadata getMetadata() const
		{
			return meta_;
		}

		void readMetadata()
		{
			std::array<char, 128> buffer;
			const std::string header = "QScalp History Data";
			stream_.read(buffer.data(), header.size());
			buffer[header.size()] = 0;
			if(header.compare(buffer.data()) != 0)
				throw std::runtime_error("Invalid header");

			int version = stream_.get();
			if(version != SupportedVersion)
				throw std::runtime_error("Unsupported version");
				
			meta_.applicationName = helpers::readString(stream_);
			meta_.comment = helpers::readString(stream_);
			meta_.startTime = helpers::readDatetime(stream_);
			meta_.streamsNumber = stream_.get();
		}

	private:
		struct StreamDescriptor
		{
			StreamId id;

			datetime_t lastTimestamp;
			union
			{
				struct
				{
					datetime_t exchangeTime;
					int64_t orderId;
					int64_t orderPrice;
					int64_t tradeId;
					int64_t tradePrice;
					int64_t openInterest;
				} ordLogState;
			};
		};

	private:
		std::istream& stream_;
		Sink& sink_;
		std::vector<StreamDescriptor> streams_;
		Metadata meta_;
	};
}

#endif
