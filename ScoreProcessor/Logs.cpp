#include "stdafx.h"
#include "Logs.h"
#include <iostream>
namespace Loggers {
	void CoutLog::log(std::string_view err,size_t)
	{
		std::cout<<err;
	}
	void CoutLog::log_error(std::string_view err,size_t)
	{
		std::cout<<err;
	}

	void AmountLog::log(std::string_view msg,size_t)
	{
		if(msg[0]=='S')
		{
			if(!begun)
			{
				begun=true;
				std::cout.write(message_template.get(),buffer_length);
			}
		}
		else
		{
			char buffer[BUFFER_SIZE];
			//if(msg[0]=='F')
			{
				auto c=++count;
				assert(c<=amount);
				//if(c==count)
				{
					insert_message(buffer,c);
					if(c==count)
					{
						std::lock_guard lock(mtx);
						if(c==count)
						{
							std::cout.write(buffer,buffer_length);
						}
					}
				}
			}
		}
	}

	void AmountLog::log_error(std::string_view msg,size_t)
	{
		std::unique_ptr<char[]> buffer(new char[msg.length()+buffer_length]);
		memcpy(buffer.get(),msg.data(),msg.length());
		size_t const c=count;
		insert_message(buffer.get()+msg.length(),c);
		if(c==count)
		{
			std::lock_guard lock(mtx);
			if(c==count)
			{
				std::cout.write(buffer.get(),msg.length()+buffer_length);
			}
			else
			{
				std::cout.write(buffer.get(),msg.length());
			}
		}
		else
		{
			std::cout.write(buffer.get(),msg.length());
		}
	}
}