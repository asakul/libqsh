
#ifndef QSHFILE_H
#define QSHFILE_H

#include <iostream>
#include <istream>
#include <memory>
#include <vector>
#include <chrono>

#include "types.h"

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
			double step;
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
			datetime_t frameTimestamp;

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

			readStreamHeaders();
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
			lastTimestamp_ = meta_.startTime / 10000;
		}

		void readStreamHeaders()
		{
			for(auto i = 0; i < meta_.streamsNumber; i++)
			{
				StreamId id;
				int type = stream_.get();
				id.type = (StreamType)type;
				auto code = helpers::readString(stream_);

				auto colon = code.find(':');
				if(colon == std::string::npos)
					throw std::runtime_error("Unable to parse security id");
				id.connector = code.substr(0, colon);
				auto start = colon + 1;

				colon = code.find(':', start);
				if(colon == std::string::npos)
					throw std::runtime_error("Unable to parse security id");
				id.ticker = code.substr(start, colon - start);
				start = colon + 1;

				colon = code.find(':', start);
				if(colon == std::string::npos)
					throw std::runtime_error("Unable to parse security id");
				id.auxcode = code.substr(start, colon - start);
				start = colon + 1;

				colon = code.find(':', start);
				if(colon == std::string::npos)
					throw std::runtime_error("Unable to parse security id");
				id.numId = std::stoi(code.substr(start, colon - start));
				start = colon + 1;

				id.step = std::stod(code.substr(start));
				start = colon + 1;

				StreamDescriptor descriptor = {};
				descriptor.id = id;

				streams_.push_back(descriptor);
			}

		}

		void readAllFrames()
		{
			while(!stream_.eof())
			{
				if(stream_.peek() == std::char_traits<char>::eof())
					break;
				readOneFrame();
			}
		}

		void readOneFrame()
		{
			auto datetime = helpers::readGrowing(stream_);
			lastTimestamp_ += datetime;
			int streamNumber = 0;
			if(streams_.size() > 1)
			{
				streamNumber = stream_.get();
			}

			currentStreamType_ = streams_[streamNumber].id.type;

			switch(currentStreamType_)
			{
				case StreamType::OrdLog:
					parseOrdLogEntry(streamNumber);
					break;
				default:
					throw std::runtime_error("Unsupported entry");
			}
		}

		void parseOrdLogEntry(int streamNumber)
		{
			auto& currentStream = streams_[streamNumber];
			int parts = stream_.get();
			uint16_t flags = stream_.get();
			flags |= ((uint16_t)stream_.get() << 8);

			if(parts & (1 << 0))
				currentStream.ordLogState.exchangeTime += helpers::readGrowing(stream_);
			if(parts & (1 << 1))
			{
				if(flags & OrderLogEntry::Add)
				{
					currentStream.ordLogState.orderId += helpers::readGrowing(stream_);
				}
				else
				{
					currentStream.ordLogState.orderId += helpers::readLeb128(stream_);
				}
			}
			if(parts & (1 << 2))
				currentStream.ordLogState.orderPrice += helpers::readLeb128(stream_);
			if(parts & (1 << 3))
				currentStream.ordLogState.volume = helpers::readLeb128(stream_);
			if(parts & (1 << 4))
				currentStream.ordLogState.volumeLeft = helpers::readLeb128(stream_);
			if(parts & (1 << 5))
				currentStream.ordLogState.tradeId += helpers::readGrowing(stream_);
			if(parts & (1 << 6))
				currentStream.ordLogState.tradePrice += helpers::readLeb128(stream_);
			if(parts & (1 << 7))
				currentStream.ordLogState.openInterest += helpers::readLeb128(stream_);

			OrderLogEntry entry;
			entry.frameTimestamp = lastTimestamp_;
			entry.flags = flags;
			entry.timestamp = currentStream.ordLogState.exchangeTime;
			entry.orderId = currentStream.ordLogState.orderId;
			double p = currentStream.ordLogState.orderPrice * currentStream.id.step;
			entry.orderPrice = decimal_fixed(floor(p), (p - floor(p)) * 1000000000);
			entry.volume = currentStream.ordLogState.volume;
			entry.remain = 0;
			if(flags & OrderLogEntry::Fill)
			{
				entry.remain = currentStream.ordLogState.volumeLeft;
			}
			else if(flags & OrderLogEntry::Add)
			{
				entry.remain = entry.volume;
			}
			entry.matchingOrderId = (flags & OrderLogEntry::Fill) ? currentStream.ordLogState.tradeId : 0;
			p = currentStream.ordLogState.tradePrice * currentStream.id.step;
			entry.tradePrice = (flags & OrderLogEntry::Fill) ? decimal_fixed(floor(p), (p - floor(p)) * 1000000000) : decimal_fixed();
			entry.openInterest = (flags & OrderLogEntry::Fill)? currentStream.ordLogState.openInterest : 0;

			sink_.orderLogFrame(entry);
		}


	private:
		struct StreamDescriptor
		{
			StreamId id;

			union
			{
				struct
				{
					datetime_t exchangeTime;
					int64_t orderId;
					int64_t orderPrice;
					int64_t volume;
					int64_t volumeLeft;
					int64_t tradeId;
					int64_t tradePrice;
					int64_t openInterest;
				} ordLogState;
			};
		};

	private:
		datetime_t lastTimestamp_;
		std::istream& stream_;
		Sink& sink_;
		std::vector<StreamDescriptor> streams_;
		Metadata meta_;
		StreamType currentStreamType_;
	};
}

#endif
