#include "stdafx.h"
#include "Logs.h"
#include <iostream>
namespace Loggers {
	void CoutLog::log(char const* msg,size_t len,size_t)
	{
		std::cout.write(msg,len);
	}
	void CoutLog::log_error(char const* msg,size_t len,size_t)
	{
		std::cout.write(msg,len);
	}

	void AmountLog::log(char const* msg,size_t len,size_t)
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

	void AmountLog::log_error(char const* msg,size_t len,size_t)
	{
		size_t const msg_buf_len=len+buffer_length;
		std::unique_ptr<char[]> buffer(new char[msg_buf_len]);
		memcpy(buffer.get(),msg,len);
		size_t const c=count;
		insert_message(buffer.get()+len,c);
		if(c==count)
		{
			std::lock_guard lock(mtx);
			if(c==count)
			{
				std::cout.write(buffer.get(),msg_buf_len);
			}
			else
			{
				std::cout.write(buffer.get(),len);
			}
		}
		else
		{
			std::cout.write(buffer.get(),len);
		}
	}
	AmountLog::~AmountLog()
	{
		std::cout<<'\n';
	}
}