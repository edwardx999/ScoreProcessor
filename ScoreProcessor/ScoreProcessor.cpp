#include "stdafx.h"
#include <iostream>
#include "Interface.h"
#include <vector>
#include <string>
#include "lib/exstring/exfiles.h"
#include "Logs.h"
#include <assert.h>
#include <unordered_set>
#include "Splice.h"
#include "lib/exstring/exiterator.h"
#ifdef MAKE_README
#include <fstream>
#endif
using namespace ScoreProcessor;

using Input = char*;
using InputIter = Input*;

constexpr bool could_be_command(char const* str)
{
	return str[0] == '-' && str[1] >= 'a' && str[1] <= 'z';
}

//could be a command but checking that it is not -r
constexpr bool could_be_command_no_rec(char const* str)
{
	if(str[0] == '-')
	{
		if(str[1] >= 'a' && str[1] <= 'z')
		{
			if(str[1] == 'r')
			{
				if(str[2] == '\0')
				{
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

//whether str is -r
constexpr bool is_rec(char const* str)
{
	return str[0] == '-' && str[1] == 'r' && str[2] == '\0';
}

//gets the date in a pretty format at compile time as an array of chars
constexpr auto date()
{
	constexpr char const* dt = __DATE__;
	constexpr size_t len = exlib::strlen(dt);
	std::array<char, len> ret{{}};
	for(size_t i = 0; i < len; ++i)
	{
		ret[i] = dt[i];
	}
	if(ret[4] == ' ') ret[4] = '0';
	return ret;
}

//outputs the help message when no args are given
void info_output()
{
	constexpr auto dt = date();
	std::cout << "Version: ";
	std::cout.write(dt.data(), dt.size());
	std::cout << " " __TIME__ " Copyright 2017-";
	std::cout.write(dt.data() + 7, 4);
	std::cout <<
		" Edward Xie\n"
		"Syntax: filename_or_folder... command params... ...\n"
		"If you want to recursively search a folder, type -r before it\n"
		"If a file starts with a dash, double the starting dash: \"-my-file.jpg\" -> \"--my-file.jpg\"\n"
		"parameters that require multiple values are notated with a comma\n"
		"parameters can be tagged to reference a specific input with prefix:value\n"
		"prefixes sometimes allow switching between different types of input\n"
		"ex: img0.png --image1.jpg my_folder -r rec_folder -fg 180 -ccg bsr:0,30 -fr l:100 w:100 h:30 t:0 -o %f.%x t\n"
		"Type command alone to get readme\n"
		"Available commands:\n"
		"  Single Page Operations:\n";
	constexpr char const* space_buffer = ":                                      ";
	constexpr size_t padding = 31;
	auto write_command = [=](auto const& it)
	{
		std::cout << "    ";
		std::cout << it.maker()->name();
		assert(padding >= it.maker()->name().length());
		std::cout.write(space_buffer, padding - it.maker()->name().length());
		std::cout << '-' << it.key() << ' ';
		std::cout << it.maker()->argument_list() << '\n';
	};
	for(auto it : ScoreProcessor::single_command_list())
	{
		write_command(it);
	}
	std::cout << "  Multi Page Operations:\n";
	for(auto it : ScoreProcessor::multi_command_list())
	{
		write_command(it);
	}
	std::cout << "  Options:\n";
	for(auto it : ScoreProcessor::option_list())
	{
		write_command(it);
	}
	std::cout << "Multiple Single Page Operations can be done at once. They are performed in the order they are given.\n"
		"A Multi Page Operation can not be done with other operations.\n";
}

#ifdef MAKE_README
//makes a readme and saves it to out
void make_readme(char const* out)
{
	std::ofstream readme(out);
	if(!readme)
	{
		return;
	}
	constexpr auto dt = date();
	readme << "An application useful in editing musical scores.  \n\n";
	readme << "Version: ";
	readme.write(dt.data(), dt.size());
	readme << " " __TIME__ " Copyright 2017-";
	readme.write(dt.data() + 7, 4);
	readme <<
		" Edward Xie\n"
		"Syntax: filename_or_folder... command params... ...  \n"
		"If you want to recursively search a folder, type -r before it  \n"
		"If a file starts with a dash, double the starting dash: \"-my-file.jpg\" -> \"--my-file.jpg\"  \n"
		"parameters that require multiple values are notated with a comma  \n"
		"parameters can be tagged to reference a specific input with prefix:value  \n"
		"prefixes sometimes allow switching between different types of input  \n"
		"ex: img0.png --image1.jpg my_folder -r rec_folder -fg 180 -ccga 20,50,30  \n"
		"Type command alone to get readme  \n"
		"Available commands:  \n"
		"&nbsp;&nbsp;Single Page Operations:  \n";
	constexpr size_t padding = 24;
	auto write_command = [&](auto it)
	{
		readme << "&nbsp;&nbsp;&nbsp;&nbsp;";
		readme << it.maker()->name();
		assert(padding >= it.maker()->name().length());
		auto padding_needed = padding - it.maker()->name().length();
		for(size_t i = 0; i < padding_needed; ++i)
		{
			readme << "&nbsp;";
		}
		readme << '-' << it.key() << ' ';
		readme << it.maker()->argument_list() << "  \n";
	};
	for(auto it : ScoreProcessor::single_command_list())
	{
		write_command(it);
	}
	readme << "&nbsp;&nbsp;Multi Page Operations:  \n";
	for(auto it : ScoreProcessor::multi_command_list())
	{
		write_command(it);
	}
	readme << "&nbsp;&nbsp;Options:  \n";
	for(auto it : ScoreProcessor::option_list())
	{
		write_command(it);
	}
	readme << "Multiple Single Page Operations can be done at once. They are performed in the order they are given.  \n"
		"A Multi Page Operation can not be done with other operations.  \n\n";

	readme <<
		"This program is free software: you can redistribute it and/or modify "
		"it under the terms of the GNU General Public License as published by "
		"the Free Software Foundation,either version 3 of the License, or "
		"(at your option) any later version.  \n"
		""
		"This program is distributed in the hope that it will be useful, "
		"but WITHOUT ANY WARRANTY; without even the implied warranty of "
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
		"GNU General Public License for more details.  \n"
		""
		"You should have received a copy of the GNU General Public License "
		"along with this program. If not, see <https://www.gnu.org/licenses/>.";
}
#endif

//displays the help message of cm
void help_output(CommandMaker const& cm)
{
	std::cout << "Command: " << cm.name() << "\n";
	if(cm.argument_list().length() != 0)
	{
		std::cout << "Args: " << cm.argument_list() << '\n';
	}
	std::cout << cm.help_message() << '\n';
}

//lists out the files in files
void list_files(std::vector<std::string> const& files)
{
	{
		std::cout << '\n';
		for(auto const& file : files)
		{
			std::cout << file << '\n';
		}
		auto const n = files.size();
		std::cout << '\n' << n << (n == 1 ? " file was found.\n" : " files were found.\n");
	}
	std::cout << '\n';
}

//finds each command key between arg_start and end, and calls the appropriate commands to change del
void parse_commands(CommandMaker::delivery& del, InputIter arg_start, InputIter end)
{
	if(arg_start != end)
	{
		auto it = arg_start + 1;
		for(;; ++it)
		{
			if(it == end || could_be_command(*it))
			{
				auto cmd = find_command((*arg_start) + 1);
				if(cmd == nullptr)
				{
					std::string err_msg("Unknown command: ");
					err_msg.append(*arg_start);
					throw std::invalid_argument(err_msg);
				}
				try
				{
					cmd->make_command(arg_start + 1, it, del);
				}
				catch(std::exception const& err)
				{
					std::string err_msg(cmd->name());
					err_msg.append(" error: ");
					err_msg.append(err.what());
					throw std::invalid_argument(err_msg);
				}
				arg_start = it;
			}
			if(it == end)
			{
				break;
			}
		}
	}
	if(del.sr.empty())
	{
		del.sr.assign("%w");
	}
}

//removes files based on the regexes and boundaries given by del
void filter_out_files(std::vector<std::string>& files, CommandMaker::delivery const& del)
{
	if(!del.selections.empty())
	{
		std::vector<char> what_to_keep(files.size(), 0); //not gonna take much space anyway, so I'm not using vector<bool>
		for(auto sr : del.selections)
		{
			auto const beg = files.begin(), ed = files.end();
			auto first = std::find_if(beg, ed, [b = sr.begin](auto const& str)
			{
				return b == std::string_view(str);
			});
			if(first == ed)
			{
				throw std::logic_error(std::string(sr.begin).append(" start boundary not found"));
			}
			auto last = std::find_if(first, ed, [e = sr.end](auto const& str)
			{
				return e == std::string_view(str);
			});
			if(last == ed)
			{
				throw std::logic_error(std::string(sr.end).append(" end boundary not found beyond ").append(sr.begin));
			}
			size_t start = first - files.begin();
			size_t const end = last - files.begin();
			for(; start <= end; ++start)
			{
				what_to_keep[start] = 1;
			}
		}
		std::vector<std::string> filtered;
		for(size_t i = 0; i < files.size(); ++i)
		{
			if(what_to_keep[i])
			{
				filtered.emplace_back(std::move(files[i]));
			}
		}
		files = std::move(filtered);
	}
	for(auto const& rgx : del.rgxes)
	{
		files.erase(std::remove_if(files.begin(), files.end(),
			[&rgx](auto const& a)
			{
				return std::regex_match(a, rgx.rgx) != rgx.keep_match;
			}), files.end());
	}
}

//returns a list of files as specified by the input from between begin and end
std::vector<std::string> get_files(InputIter begin, InputIter end)
{
	bool do_recursive = false;
	std::vector<std::string> files;
	for(auto pos = begin; pos != end; ++pos)
	{
		if(is_rec(*pos))
		{
			if(do_recursive)
			{
				throw std::logic_error("Double recursive flag given");
			}
			do_recursive = true;
		}
		else
		{
			auto fixed_path =
				((*pos)[0] == '-' && (*pos)[1] == '-') ?
				(*pos) + 1 :
				*pos;
			auto file_attr = GetFileAttributesA(fixed_path);
			if(file_attr == INVALID_FILE_ATTRIBUTES)
			{
				std::string err_msg("File or folder not found: ");
				err_msg.append(fixed_path);
				throw std::invalid_argument(err_msg);
			}
			if(file_attr & FILE_ATTRIBUTE_DIRECTORY)
			{

				std::string path(fixed_path);
				if(path.back() != '/' && path.back() != '\\')
				{
					path += '\\';
				}
				auto fid = do_recursive ? exlib::files_in_dir_rec(path) : exlib::files_in_dir(path);
				if(path == "./" || path == ".\\")
				{
					files.insert(files.end(), std::make_move_iterator(fid.begin()), std::make_move_iterator(fid.end()));
				}
				else
				{
					files.reserve(files.size() + fid.size());
					for(auto const& str : fid)
					{
						std::string name(path);
						name.append(str);
						files.emplace_back(std::move(name));
					}
				}
				do_recursive = false;
			}
			else
			{
				if(do_recursive)
				{
					std::string err_msg("Cannot recursively search non-folder: ");
					err_msg.append(fixed_path);
					throw std::logic_error(err_msg);
				}
				files.emplace_back(std::move(fixed_path));
			}
		}
	}
	if(do_recursive)
	{
		throw std::logic_error("Recursive flag given at end of input list");
	}
	return files;
}

//finds the end of the file input list/the beginning of the command input list
InputIter find_file_list(InputIter begin, InputIter end)
{
	for(auto pos = begin; pos != end; ++pos)
	{
		if(could_be_command_no_rec(*pos)) return pos;
	}
	return end;
}

//applies the single image processes
void do_single(CommandMaker::delivery const& del, std::vector<std::string> const& files)
{
	del.pl.process(files, &del.sr, del.num_threads, del.starting_index, del.do_move, del.quality);
}

//applies the cut process to the images
void do_cut(CommandMaker::delivery const& del, std::vector<std::string> const& files)
{
	using pv = decltype(CommandMaker::delivery::cut_args.min_width);
	struct cut_args {
		SaveRules const* output;
		int verbosity;
		unsigned char background;
		pv min_width;
		pv min_height;
		pv min_vert_space;
		float horiz_weight;
		Log* log;
		int quality;
	};
	class CutProcess {
	private:
		std::string const* input;
		unsigned int index;
	public:
		CutProcess(std::string const* input, unsigned int index, CommandMaker::delivery const& del):
			input(input),
			index(index)
		{}
		void execute(cut_args const* ca) const
		{
			try
			{
				if(ca->verbosity > ProcessList<>::verbosity::errors_only)
				{
					std::string coutput("Starting ");
					coutput.append(*input);
					coutput.append(1, '\n');
					ca->log->log(coutput.c_str(), index);
				}
				auto out = ca->output->make_filename(*input, index);
				auto ext = exlib::find_extension(out.begin(), out.end());
				auto const s = validate_extension(ext);
				cil::CImg<unsigned char> in(input->c_str());
				cut_heuristics cut_args;
				cut_args.background = ca->background;
				cut_args.horizontal_energy_weight = ca->horiz_weight;
				std::array<unsigned int, 2> bases{in._width,in._height};
				cut_args.min_height = (ca->min_height)(bases);
				cut_args.min_width = ca->min_width(bases);
				cut_args.minimum_vertical_space = ca->min_vert_space(bases);
				auto num_pages = ScoreProcessor::cut_page(in, out.c_str(), cut_args, ca->quality);
				if(ca->verbosity > ProcessList<>::verbosity::errors_only)
				{
					std::string coutput("Finished ");
					coutput.append(*input);
					coutput.append(" and created ");
					coutput.append(std::to_string(num_pages));
					coutput.append(num_pages == 1 ? " page\n" : " pages\n");
					ca->log->log(coutput.c_str(), index);
				}
			}
			catch(std::exception const& ex)
			{
				if(ca->verbosity > ProcessList<>::verbosity::silent)
				{
					std::string err(ex.what());
					err += '\n';
					ca->log->log_error(err.c_str(), index);
				}
			}
		}
	};
	cut_args ca{
	&del.sr,
	del.pl.get_verbosity(),
	del.cut_args.background,
	del.cut_args.min_width,
	del.cut_args.min_height,
	del.cut_args.min_vert_space,
	del.cut_args.horiz_weight,
	del.pl.get_log(),
	del.quality
	};
	exlib::thread_pool_a<cut_args const*> tp(del.num_threads, &ca);
	for(size_t i = 0; i < files.size(); ++i)
	{
		tp.push_back([process = CutProcess{&files[i],static_cast<unsigned int>(i + del.starting_index),del}](cut_args const* ca) noexcept {
			process.execute(ca);
		});
	}
}

//applies the splice process
void do_splice(CommandMaker::delivery const& del, std::vector<std::string> const& files)
{
	try
	{
		auto save = del.sr.make_filename(files[0], del.starting_index);
		auto ext = exlib::find_extension(save.begin(), save.end());
		validate_extension(ext);
		Splice::standard_heuristics sh;
		auto num = del.splice_divider.data() ?
			splice_pages_parallel(files, save.c_str(), del.starting_index, del.num_threads, del.splice_args, del.splice_divider, del.quality) :
			splice_pages_parallel(files, save.c_str(), del.starting_index, del.num_threads, del.splice_args, del.quality);
		std::cout << "Created " << num << (num == 1 ? " page\n" : " pages\n");
	}
	catch(std::exception const& ex)
	{
		std::cout << "Error(s):\n" << ex.what() << '\n';
	}
}

namespace std {
	template<>
	struct hash<std::filesystem::path> {
		template<typename Path>
		std::size_t operator()(Path const& path) const noexcept
		{
			return hash_value(path);
		}
	};
}
template<typename PathS = void, typename Iter, typename Sentinal>
bool find_collisions(Iter begin, Sentinal end)
{
	using Path = typename std::conditional<
		std::is_void<PathS>::value,
		typename std::decay<decltype(*begin)>::type,
		PathS>::type;
	std::vector<std::pair<Path, std::unordered_set<Path>>> collisions;
	using PathRef = typename std::remove_cv<typename exlib::remove_rvalue_reference<decltype(*begin)>::type>::type;
	for(; begin != end; ++begin)
	{
		PathRef entry = *begin;
		auto dir = entry.parent_path();
		if(dir.empty())
		{
			dir = ".";
		}
		auto filename = entry.filename();
		bool found = false;
		for(auto& known_dir : collisions)
		{
			if(known_dir.first == dir || equivalent(known_dir.first, dir))
			{
				found = true;
				auto res = known_dir.second.insert(std::move(filename));
				if(!res.second)
				{
					std::cout << "Collision between \"" + entry.string() + "\" and \"" + (known_dir.first / *res.first).string() + "\"\n";
					return true;
				}
			}
		}
		if(!found)
		{
			collisions.push_back({std::move(dir),{}});
			collisions.back().second.insert(std::move(filename));
		}
	}
	return false;
}

template<typename PathsIter>
bool has_collisions(PathsIter begin, PathsIter end, SaveRules const& rules, unsigned int si)
{
	auto transformer = [&rules, &si](std::string const& input)
	{
		auto path = rules.make_filename(input, si++);
		return std::filesystem::path(path);
	};
	auto tbegin = exlib::transform_iterator{begin, transformer};
	auto tend = decltype(tbegin){end, transformer};
	return find_collisions(tbegin, tend);
}
int main(int argc, InputIter argv)
{
#ifdef MAKE_README
	if(argc > 1)
		make_readme(argv[1]);
	return 0;
#endif
	std::ios::sync_with_stdio(false);
	if(argc == 1)
	{
		info_output();
		return 0;
	}
	cil::cimg::exception_mode(0);
	if(could_be_command_no_rec(argv[1]))
	{
		auto cmd = find_command(argv[1] + 1);
		if(cmd != nullptr)
		{
			help_output(*cmd);
		}
		else
		{
			std::cout << "Unknown command: " << argv[1] << '\n';
		}
		return 0;
	}
	CommandMaker::delivery del;
	auto start = argv + 1;
	auto end = argv + argc;
	auto const file_end = find_file_list(start, end);
	std::vector<std::string> files;
	try
	{
		parse_commands(del, file_end, end);
		if(del.flag == del.do_absolutely_nothing && !del.list_files)
		{
			std::cout << "No commands given\n";
			return 0;
		}
		files = get_files(start, file_end);
		filter_out_files(files, del);
	}
	catch(std::exception const& ex)
	{
		std::cout << ex.what() << '\n';
		return 1;
	}
	if(files.empty())
	{
		std::cout << "No files were found.\n";
		return 0;
	}
	if(del.list_files)
	{
		list_files(files);
	}
	del.fix_values(files.size());
	if(del.flag != del.do_splice && has_collisions(files.begin(), files.end(), del.sr, del.starting_index))
	{
		std::cout << "Collision in output names\n";
		return 1;
	}
	std::optional<Loggers::AmountLog> al;
	Loggers::CoutLog cl;
	switch(del.lt)
	{
	case del.unassigned_log:
	case del.count:
		al.emplace(files.size());
		del.pl.set_log(&*al);
		del.pl.set_verbosity(del.pl.loud);
		break;
	case del.errors_only:
		del.pl.set_log(&cl);
		del.pl.set_verbosity(del.pl.errors_only);
		break;
	case del.quiet:
		del.pl.set_log(&cl);
		del.pl.set_verbosity(del.pl.silent);
		break;
	case del.full_message:
		del.pl.set_log(&cl);
		del.pl.set_verbosity(del.pl.loud);
	}
	switch(del.flag)
	{
	case del.do_absolutely_nothing:
		std::cout << "No commands given\n";
		break;
	case del.do_nothing:
		[[fallthrough]];
	case del.do_single:
		do_single(del, files);
		break;
	case del.do_cut:
		do_cut(del, files);
		break;
	case del.do_splice:
		do_splice(del, files);
		break;
	}
	return 0;
}