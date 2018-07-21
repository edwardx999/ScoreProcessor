#ifndef LOGS_H
#define LOGS_H
#include "ImageProcess.h"
#include "lib/exstring/exmath.h"
#include "lib/exstring/exstring.h"
namespace Loggers {
	class CoutLog:public ScoreProcessor::Log {
	public:
		void log(std::string_view msg,size_t) override;
		void log_error(std::string_view msg,size_t) override;
	};

	class AmountLog:public ScoreProcessor::Log {
	private:
		std::atomic<size_t> count;//count of finished pages
		size_t amount;
		unsigned int num_digs;
		size_t buffer_length;
		std::unique_ptr<char[]> message_template;
		std::mutex mtx;
		bool begun;//atomicity not needed, double writing the first thing is ok
		static constexpr char const* START_MSG="Finished ";
		static constexpr size_t const START_STRLEN=exlib::strlen(START_MSG);
		static constexpr size_t const EXTRA_CHARS=2;//slash and carriage return
		static constexpr size_t const BUFFER_SIZE=START_STRLEN+EXTRA_CHARS+2*exlib::num_digits(std::numeric_limits<size_t>::max());//enough to fit message and digits of max value of size_t
		inline static void insert_numbers(char* place,size_t num)
		{
			while(true)
			{
				*place=num%10+'0';
				num/=10;
				if(num==0)
				{
					break;
				}
				--place;
			}
		}
		inline void insert_message(char* place,size_t num)
		{
			memcpy(place,message_template.get(),buffer_length);
			char* it=place+START_STRLEN+num_digs-1;
			insert_numbers(it,num);
		}
	public:
		inline AmountLog(size_t amount):
			amount(amount),
			count(0),
			num_digs(exlib::num_digits(amount)),
			buffer_length(START_STRLEN+EXTRA_CHARS+2*num_digs),
			message_template(new char[buffer_length]),
			begun(false)
		{
			memcpy(message_template.get(),START_MSG,START_STRLEN);
			size_t const end=START_STRLEN+num_digs-1;
			for(size_t i=START_STRLEN;i<end;++i)
			{
				message_template[i]=' ';
			}
			message_template[end]='0';
			message_template[START_STRLEN+num_digs]='/';
			insert_numbers(message_template.get()+buffer_length-2,amount);
			message_template[START_STRLEN+1+2*num_digs]='\r';
		}
		void log(std::string_view msg,size_t) override;
		void log_error(std::string_view msg,size_t) override;
	};
}
#endif