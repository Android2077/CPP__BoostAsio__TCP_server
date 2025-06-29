#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <deque> 
#include <list> 
#include <set>
#include <chrono>
#include <functional>
#include <memory>

#include <thread>
#include <mutex>              
#include <condition_variable> 



#define BOOST_SYSTEM_USE_UTF8          //Нужен для того, чтобы Boost Asio возвращал текст ошибок в кодировке UTF-8.

#include <boost\asio.hpp>      //путь к папке boost - указан в свойствах проекта.



class BoostAsio__TCP_server
{

private:

	class ThreadPoolTask__class
	{

	public:

		enum error_enum : int
		{
			bad_alloc = 1,
			other_error = 2,
		};

	public:


		void set_lambda_error(std::function<void(const error_enum error_enum_, const std::string error)>user_callback_error_)
		{
			std::lock_guard<std::mutex> lock(mutex_);

			user_callback_error = user_callback_error_;
		}




		void set_threads(const int number_threads)
		{
			delete_all_threads();                                                                               //Удаляем все потоки и список функций работющий в них и просто устанавливаем с нуля.


			std::lock_guard<std::mutex> lock(mutex_);                                                          //Блокируем мьютекс                  


			//------------------------------------------------------------------------
			try { list_treads.resize(number_threads); }
			catch (const std::bad_alloc& e) { const std::string temp = e.what(); if (user_callback_error != 0) { user_callback_error(error_enum::bad_alloc, "set_threads:resize_bad_alloc:" + temp); } }


			try
			{
				for (std::list<thread_struct>::iterator it = list_treads.begin(); it != list_treads.end(); ++it)
				{
					(*it).release_func_flag = false;
					(*it).thread_ = std::thread(&ThreadPoolTask__class::working_threads, this, this, std::ref((*it).release_func_flag));  //Запускаем функцию мониторинга очереди задач и выполенния этих задач в потоке.
				}
			}
			catch (const std::bad_alloc& e)
			{
				const std::string temp = e.what(); if (user_callback_error != 0) { user_callback_error(error_enum::other_error, "set_threads:" + temp); }
			}

			//------------------------------------------------------------------------

		}

		void add_threads(const int number_threads)
		{

			std::lock_guard<std::mutex> lock(mutex_);                                                    //Блокируем мьютекс


			//------------------------------------------------------------------------
			for (int i = 0; i < number_threads; ++i)
			{
				try { list_treads.emplace_back(); }
				catch (const std::bad_alloc& e) { const std::string temp = e.what(); if (user_callback_error != 0) { user_callback_error(error_enum::bad_alloc, "add_threads:emplace_back_bad_alloc:" + temp); } }

				try
				{
					list_treads.back().release_func_flag = false;
					list_treads.back().thread_ = std::thread(&ThreadPoolTask__class::working_threads, this, this, std::ref(list_treads.back().release_func_flag));
				}
				catch (const std::bad_alloc& e)
				{
					const std::string temp = e.what(); if (user_callback_error != 0) { user_callback_error(error_enum::other_error, "set_threads:" + temp); }
				}
			}
			//------------------------------------------------------------------------

		}

		void add_task(std::function<void()> f_object)
		{

			std::lock_guard<std::mutex> lock(mutex_);                                                    //Блокируем мьютекс


			//-------------------------------------------------------------------------
			try { deque_fobject.push_back(std::move(f_object)); }
			catch (const std::bad_alloc& e) { const std::string temp = e.what(); if (user_callback_error != 0) { user_callback_error(error_enum::bad_alloc, "add_task:push_back_bad_alloc:" + temp); } }

			CV_.notify_one();
			//-------------------------------------------------------------------------

		}



		void delete_threads(const int number_threads)
		{

			std::lock_guard<std::mutex> lock(mutex_for_delete_threads);         //ПОДРОБНО: функция удаляет опрделенное указанное "number_threads" кол-во потоков. Блокируем общий мьютекс, проверяем, если list_treads не равен нулю, то ставим у ервого элемента у поля release_func_flag == true, чтобы функция "working_threads" которая крутится в цикле в отдельном рабочем потоке проверила, если release_func_flag == true - то завершаем цикл, и выходим из функции и тем самым завершаем поток. Для этого будим все потоки, чтобы тот поток у которого мы изменили release_func_flag на true - вышел из цикла и мы дождались завершения потока через join(), НО!!! предположим поток спал, мы заблкировали мьютекс и иизменили release_func_flag на true, теперь мы пробудили поток и пробужденный поток должен у себя в функции "working_threads" захватить мьютекс, который мы заблокировали, то есть чтобы дождатся завершение join потока - нам придется освободить мьютекс и дожидатся заврешенеи потока не из под мьютекса И ТУТ получается фигня, так как "list_treads" - становится уязвим к гонке данных, если кто-то вызвет данную функцию этого обьекта класса из друого потока, ПОЭТОМУ чтобы такого не случилось и введен дополнительный мьютекс, который прямо на входе функции будет блокировать попытки вызвать паралльно эту функцию из других потоков.


			for (int i = 0; i < number_threads; ++i)
			{
				//------------------------------------------------------------
				{
					std::unique_lock<std::mutex> my_lock(mutex_);       //Блокируем мьютекс

					if (list_treads.size() != 0)
					{
						list_treads.front().release_func_flag = true;
					}

					CV_.notify_all();                                   //Пробуждаем все потоки, а не один через notify_one(), так как флаг "release_func_flag = true" устаовленный для первого элемента list_treads - не обязательно будет связан с пробужденным потоком через "notify_one()", так как notify_one() - может пробудет любой поток, а не тот которого я поставил флаг "release_func_flag = true" - поэтому нужно пробуждать все, чтобы тот у кторого я поставил флаг в итоге заверщил свою работу и мы дождались его через join().
				}
				//------------------------------------------------------------


				list_treads.front().thread_.join();  				// Ожидаем завершения потока, который находится в первом элементе list_treads. Защищен мьютексом "mutex_for_delete_threads".


				//------------------------------------------------------------
				{
					std::unique_lock<std::mutex> my_lock(mutex_);       //Опять Блокируем мьютекс

					list_treads.pop_front();                        	// Удаляем поток из списка
				}
				//------------------------------------------------------------

			}

		}

		void delete_all_threads()
		{

			std::lock_guard<std::mutex> lock(mutex_for_delete_threads);          //ПОДРОБНО: читать в фукнции "delete_threads"


			//---------------------------------------------------------------------------------------------
			{
				std::unique_lock<std::mutex> lock(mutex_);

				for (std::list<thread_struct>::iterator it = list_treads.begin(); it != list_treads.end(); ++it)
				{
					(*it).release_func_flag = true;        //Установим флаг в "true" для функции run_loop, что пора завершать созданнй поток.
				}

				CV_.notify_all();                           //будим все потоки, если они находится в ожидании.
			}
			//---------------------------------------------------------------------------------------------



			//--------------------------------Защищен мьютексом "mutex_for_delete_threads"-----------------
			for (std::list<thread_struct>::iterator it = list_treads.begin(); it != list_treads.end(); ++it)
			{
				(*it).thread_.join();
			}
			//---------------------------------------------------------------------------------------------



			//------------------------------------------------------------
			{
				std::unique_lock<std::mutex> lock(mutex_);           //Теоретически можно тут не захватывать мьютекс, так как все потоки завершены и никто уже не будет образтся к "list_treads" -  но пусть будет на всякай случай.

				list_treads.clear();
			}
			//------------------------------------------------------------

		}




		const size_t get__deque_size()
		{
			std::lock_guard<std::mutex> lock(mutex_);

			return deque_fobject.size();
		}

		std::mutex& get__mutex_ref()
		{
			return mutex_;
		}

		void AllFree()
		{
			delete_all_threads();

			{
				std::unique_lock<std::mutex> lock(mutex_);           //Теоретически можно тут не захватывать мьютекс, так как все потоки завершены и никто уже не будет образтся к "list_treads" -  но пусть будет на всякай случай.

				deque_fobject.clear();
			}

		}


		~ThreadPoolTask__class()
		{
			AllFree();
		}







	private:


		struct thread_struct
		{
			std::thread thread_;
			bool release_func_flag = false;

		};
		std::list<thread_struct>list_treads;
		std::mutex mutex_for_delete_threads;         //Значем нужен этот мьютекст читать в функции "delete_threads"

		std::mutex mutex_;
		std::condition_variable CV_;

		std::deque <std::function<void()>> deque_fobject;

		std::function<void(const error_enum error_enum_, const std::string error)>user_callback_error = 0;

		//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


		void working_threads(ThreadPoolTask__class* class_p, const bool& flag_close__ref)
		{

			std::function <void()> f_object;


			while (true)
			{

				//----------------------------------------------------------------
				{
					std::unique_lock<std::mutex> my_lock((*class_p).mutex_);                                                    //Блокируем мьютекс


					while (flag_close__ref == false && (*class_p).deque_fobject.size() == 0)                               //Проверяем, если ли активные задачи в очереди, которые еще не взяты ни одним потоком.
					{
						//Задач, которые не взяты - нет. Можно засыпать.

						(*class_p).CV_.wait(my_lock);                                  //Усыпляем поток в ожидании пробуждения со строны Пользователя. Мьютекс освобождается.
					}


					//Поток пробудился и готов выполнять работу:



					//----------------------------------------------
					if (flag_close__ref == true)
					{
						break; 		//Значит Пользователь вызвал завершения потока или вызвался десктрутор. Выходим полностью из цикла.
					}
					//----------------------------------------------


					f_object = std::move((*class_p).deque_fobject.front());       //Достаем функцию из очереди.

					(*class_p).deque_fobject.pop_front();                         //Удаляем задачу из очереди.

				}
				//----------------------------------------------------------------


				f_object();     //Выполянем Пользовательскую фунцуию.

			}

		}


	};

public:

	enum error_enum : int
	{
		boost_error = 1,
		bad_alloc = 2,
		other_error = 3,
	};

	enum ThreadType_enum
	{
		main_thread = 1,
		multi_threads = 2,
	};

	enum class CallbackMessage : int
	{
		Part = 1,
		Full = 2,
	};

	enum class CallbackMode : int
	{
		Call_AlwaysPart_Send = 1,
		Call_OnlyFull_Send = 2,
	};

	enum class Connect_flag : int
	{
		Connect = 1,
		Disconnect = 2,
	};



public:


	class Sockets__class
	{


	public:

		struct socket_struct
		{
			//-------------------------------------------------------------------------
			boost::asio::ip::tcp::socket socket_;
			//-------------------------------------------------------------------------

			//-------------------------------------------------------------------------
			std::list<socket_struct>::iterator it_this;   //Итератор на данный элемент в "list_socket"
			//-------------------------------------------------------------------------



			//**********************************Расширемые данные:Начало***************************************************

			bool Connect_flag = false;


			struct Read_struct
			{

				Read_struct()
				{
					BufferReceiver.resize(1024);  //Размер по умолчанию.
				}

				void set__SizeBuff_for_Read(const size_t size_)
				{
					BufferReceiver.resize(size_);
				}

				struct read_untill
				{
					std::string string_buff;
				}read_untill_;

				//-------------------------------------
				std::string BufferReceiver;
				//-------------------------------------

			}Read_struct_;



			struct Write_struct
			{

				//---------------------------------
				struct UserBuff_struct
				{
					char* pointer_to_Buff = 0;
					size_t Buff_size = 0;
				}UserBuff_struct_;

				std::string Data_copy;
				//---------------------------------


				//*************************************Служебные:*******************************************

				//-----------------------------------------------------------------
				size_t bytes_send_transferred = 0;               //Какое кол - во байт из "BuffToSend__size" переданно на данный момент.Это служебная переменная, для того, чтобы определять момент когда все данные по задаче Пользователя переданы.
				//-----------------------------------------------------------------


				//-------------------для механизма последовательности выполнения задач отправки:----------------------------------------------
				bool Full_Transfered__flag = true;

				std::deque<std::function<void()>>deque_for_Sequential_Send;
				//-----------------------------------------------------------------------------------------------------------------


			}Write_struct_;



			//**********************************Расширемые данные:Конец***************************************************


			socket_struct(boost::asio::ip::tcp::socket&& socket__) : socket_(std::move(socket__)) {}
			//socket_struct(const std::string& Socket_name_, boost::asio::ip::tcp IP_type_, boost::asio::ip::tcp::socket&& socket__) : Socket_name(Socket_name_), IP_type(IP_type_), socket_(std::move(socket__)) {}
		};

	public:

		Sockets__class(boost::asio::io_context* io_context_p_, std::function<void(Sockets__class* Sockets__class_p, socket_struct* socket_struct_p, const error_enum error_enum_, const boost::system::error_code& boost_error, const std::string& text_error)> lambda_error_)
		{
			io_context_p = io_context_p_;

			lambda_error = lambda_error_;
		}


	public:




	public:

		socket_struct* add__NewSocket(const boost::asio::ip::tcp IP_type)
		{


			//----------------------------------Создаем и открываем Новый Сокет:Начало----------------------------------------
			boost::system::error_code boost_error_code;

			boost::asio::ip::tcp::socket socket(*io_context_p);

			//Сокет открывать не нужно, так как async_accept - требуется, чтобы сокет был закрыт, так как он сам его открыает.
			//----------------------------------Создаем и открываем Новый Сокет:Конец----------------------------------------




			try
			{
				//--------------------------------------------------------------------------
				list_socket.emplace_back(std::move(socket));

				socket_struct* socket_struct_p = &list_socket.back();

				socket_struct_p->it_this = std::prev(list_socket.end());

				//UnorderedMap_SocketName.emplace(Socket_name, socket_struct_p);         //Добавляем соответсвие имени сокета и указателя на элемент структуры с сокетом, для быстрого поиска по Имени Сокета.
				//--------------------------------------------------------------------------


				return socket_struct_p;
			}
			catch (std::bad_alloc& e)
			{
				boost::system::error_code boost_error_code;  //Просто затычка для вызова "ambda_error"

				lambda_error(this, 0, error_enum::bad_alloc, boost_error_code, "add__NewSocket:bad_aloc:" + (std::string)e.what());

				return 0;
			}
			catch (std::exception& e)
			{
				boost::system::error_code boost_error_code;  //Просто затычка для вызова "ambda_error"

				lambda_error(this, 0, error_enum::boost_error, boost_error_code, "add__NewSocket:" + (std::string)e.what());

				return 0;
			}

		}


		void close_Socket(socket_struct* socket_struct_p)
		{
			if (socket_struct_p != 0)
			{
				if (socket_struct_p->socket_.is_open() == true)
				{
					socket_struct_p->socket_.close();

					try
					{
						deque_SocketsClosed.push_back(socket_struct_p);
					}
					catch (std::bad_alloc& e)
					{
						boost::system::error_code boost_error_code;  //Просто затычка для вызова "ambda_error"

						lambda_error(this, 0, error_enum::bad_alloc, boost_error_code, "close_Socket:bad_aloc:" + (std::string)e.what());
					}
				}

			}
		}

		socket_struct* open_Previously_ClosedSocket(const boost::asio::ip::tcp IP_type)
		{

			if (deque_SocketsClosed.size() > 0)
			{
				socket_struct* socket_struct_p = deque_SocketsClosed.front();

				deque_SocketsClosed.pop_front();

				//Сокет открывать не нужно, так как async_accept - требуется, чтобы сокет был закрыт, так как он сам его открыает.

				return socket_struct_p;
			}

			return 0;
		}

		const bool redefine_socket(socket_struct* socket_struct_p, const boost::asio::ip::tcp IP_type)
		{

			socket_struct_p->socket_.close();

			//-----------------------------------------------------------------------
			boost::system::error_code boost_error_code;

			socket_struct_p->socket_.open(IP_type, boost_error_code);

			if (boost_error_code.value() != boost::system::errc::success)
			{
				lambda_error(this, socket_struct_p, error_enum::boost_error, boost_error_code, "redefine_socket:open():" + get__BoostErrorText(boost_error_code.value()) + ":" + boost_error_code.message());

				return false;
			}
			//-----------------------------------------------------------------------

			return true;
		}

		void close_AllSocket()
		{

			try
			{
				for (std::list<socket_struct>::iterator it = list_socket.begin(); it != list_socket.end(); ++it)
				{
					it->socket_.close();

					deque_SocketsClosed.push_back(&(*it));
				}
			}
			catch (std::bad_alloc& e)
			{
				boost::system::error_code boost_error_code;  //Просто затычка для вызова "ambda_error"

				lambda_error(this, 0, error_enum::bad_alloc, boost_error_code, "close_Socket:bad_aloc:" + (std::string)e.what());
			}

		}
		void delete_AllSocket()
		{
			close_AllSocket();

			list_socket.clear();
			deque_SocketsClosed.clear();
		}



	private:

		boost::asio::io_context* io_context_p;

		//----------------------------------------------------------
		std::list<socket_struct> list_socket;

		std::deque<socket_struct*>deque_SocketsClosed;
		//----------------------------------------------------------


		std::function<void(Sockets__class* Sockets__class_p, socket_struct* socket_struct_p, const error_enum, const boost::system::error_code& boost_error, const std::string& text_error)> lambda_error;


		//****************************************************************************************************************************************************


		const std::string get__BoostErrorText(const int boost_ErrorCode)
		{

			if (boost_ErrorCode == boost::asio::error::access_denied)
			{
				return "access_denied";
			}
			if (boost_ErrorCode == boost::asio::error::address_family_not_supported)
			{
				return "address_family_not_supported";
			}
			if (boost_ErrorCode == boost::asio::error::address_in_use)
			{
				return "address_in_use";
			}
			if (boost_ErrorCode == boost::asio::error::already_connected)
			{
				return "already_connected";
			}
			if (boost_ErrorCode == boost::asio::error::already_open)
			{
				return "already_open";
			}
			if (boost_ErrorCode == boost::asio::error::already_started)
			{
				return "already_started";
			}
			if (boost_ErrorCode == boost::asio::error::bad_descriptor)
			{
				return "bad_descriptor";
			}
			if (boost_ErrorCode == boost::asio::error::broken_pipe)
			{
				return "broken_pipe";
			}
			if (boost_ErrorCode == boost::asio::error::connection_aborted)
			{
				return "connection_aborted";
			}
			if (boost_ErrorCode == boost::asio::error::connection_refused)
			{
				return "connection_refused";
			}
			if (boost_ErrorCode == boost::asio::error::connection_reset)
			{
				return "connection_reset";
			}
			if (boost_ErrorCode == boost::asio::error::eof)
			{
				return "eof";
			}
			if (boost_ErrorCode == boost::asio::error::fault)
			{
				return "fault";
			}
			if (boost_ErrorCode == boost::asio::error::fd_set_failure)
			{
				return "fd_set_failure";
			}
			if (boost_ErrorCode == boost::asio::error::host_not_found)
			{
				return "host_not_found";
			}
			if (boost_ErrorCode == boost::asio::error::host_not_found_try_again)
			{
				return "host_not_found_try_again";
			}
			if (boost_ErrorCode == boost::asio::error::host_unreachable)
			{
				return "host_unreachable";
			}
			if (boost_ErrorCode == boost::asio::error::interrupted)
			{
				return "interrupted";
			}
			if (boost_ErrorCode == boost::asio::error::invalid_argument)
			{
				return "invalid_argument";
			}
			if (boost_ErrorCode == boost::asio::error::in_progress)
			{
				return "in_progress";
			}
			if (boost_ErrorCode == boost::asio::error::message_size)
			{
				return "message_size";
			}
			if (boost_ErrorCode == boost::asio::error::name_too_long)
			{
				return "name_too_long";
			}
			if (boost_ErrorCode == boost::asio::error::network_down)
			{
				return "network_down";
			}
			if (boost_ErrorCode == boost::asio::error::network_reset)
			{
				return "network_reset";
			}
			if (boost_ErrorCode == boost::asio::error::network_unreachable)
			{
				return "network_unreachable";
			}
			if (boost_ErrorCode == boost::asio::error::not_connected)
			{
				return "not_connected";
			}
			if (boost_ErrorCode == boost::asio::error::not_found)
			{
				return "not_found";
			}
			if (boost_ErrorCode == boost::asio::error::not_socket)
			{
				return "not_socket";
			}
			if (boost_ErrorCode == boost::asio::error::no_buffer_space)
			{
				return "no_buffer_space";
			}
			if (boost_ErrorCode == boost::asio::error::no_data)
			{
				return "no_data";
			}
			if (boost_ErrorCode == boost::asio::error::no_descriptors)
			{
				return "no_descriptors";
			}
			if (boost_ErrorCode == boost::asio::error::no_memory)
			{
				return "no_memory";
			}
			if (boost_ErrorCode == boost::asio::error::no_permission)
			{
				return "no_permission";
			}
			if (boost_ErrorCode == boost::asio::error::no_protocol_option)
			{
				return "no_protocol_option";
			}
			if (boost_ErrorCode == boost::asio::error::no_recovery)
			{
				return "no_recovery";
			}
			if (boost_ErrorCode == boost::asio::error::no_such_device)
			{
				return "no_such_device";
			}
			if (boost_ErrorCode == boost::asio::error::operation_not_supported)
			{
				return "operation_not_supported";
			}
			if (boost_ErrorCode == boost::asio::error::operation_aborted)
			{
				return "operation_aborted";
			}
			if (boost_ErrorCode == boost::asio::error::service_not_found)
			{
				return "service_not_found";
			}
			if (boost_ErrorCode == boost::asio::error::shut_down)
			{
				return "shut_down";
			}
			if (boost_ErrorCode == boost::asio::error::socket_type_not_supported)
			{
				return "socket_type_not_supported";
			}
			if (boost_ErrorCode == boost::asio::error::timed_out)
			{
				return "timed_out";
			}
			if (boost_ErrorCode == boost::asio::error::try_again)
			{
				return "try_again";
			}
			if (boost_ErrorCode == boost::asio::error::would_block)
			{
				return "would_block";
			}

			return std::to_string(boost_ErrorCode);

		}
	};


public:


	struct acceptor_struct
	{

	public:

		std::string Acceptor_Name;

		//-------------------------------------------------
		boost::asio::ip::tcp::acceptor acceptor_;

		boost::asio::ip::tcp IP_type;

		boost::asio::ip::tcp::endpoint endpoint_;
		//-------------------------------------------------



		//-------------------------------------------------
		struct Read_struct
		{
			struct read_untill
			{
				std::string user_separator;
			}read_untill_;

			size_t Recieve_SizeBuf = 1024;  //По умолчанию.

		}Read_struct_;
		//-------------------------------------------------

		//-------------------------------------------------
		struct Write_struct
		{
			CallbackMode CallbackMode_ = CallbackMode::Call_AlwaysPart_Send;
		}Write_struct_;
		//-------------------------------------------------




		Sockets__class* get__SocketClass_pointer()
		{
			return Sockets__class_uptr.get();
		}

		std::unique_ptr<Sockets__class> Sockets__class_uptr;


		//***********************************************************************************************************************************************

		acceptor_struct(const std::string& Acceptor_Name_, boost::asio::ip::tcp::acceptor&& acceptor__, const boost::asio::ip::tcp IP_type_, boost::asio::ip::tcp::endpoint&& endpoint__) : Acceptor_Name(Acceptor_Name_), acceptor_(std::move(acceptor__)), IP_type(IP_type_), endpoint_(std::move(endpoint__)) {}
	};


public:

	BoostAsio__TCP_server(const ThreadType_enum ThreadType_enum_, std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_server_p, acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, const error_enum error_enum_, const boost::system::error_code& boost_error, const std::string& error_string)> lambda_error_)
	{

		work_guard_uptr.reset(new boost::asio::executor_work_guard<boost::asio::io_context::executor_type>(io_context.get_executor()));


		ThreadType_enum = ThreadType_enum_;

		if (ThreadType_enum_ == ThreadType_enum::multi_threads)
		{
			//------------------------------------------------------------------
			num_threads = std::thread::hardware_concurrency();

			if (num_threads == 0)       //На некоторых системах hardware_concurrency() может выдавать 0, поэтому, если это произойдет, то просто поставим 1 поток.
			{
				num_threads = 1;
			}

			vec_thread.resize(num_threads);

			for (int i = 0; i < num_threads; i++)
			{
				vec_thread[i] = std::thread(&BoostAsio__TCP_server::io_context__Run, this);
			}
			//------------------------------------------------------------------
		}


		lambda_error = lambda_error_;



		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		ThreadPoolTask__uptr.reset(new ThreadPoolTask__class);

		ThreadPoolTask__uptr.get()->set_threads(1);

		ThreadPoolTask__uptr.get()->set_lambda_error([this](const ThreadPoolTask__class::error_enum error_enum_, const std::string error)
			{
				boost::system::error_code boost_error_code;  //Просто затычка для вызова "ambda_error"

				if (error_enum_ == ThreadPoolTask__class::error_enum::bad_alloc)
				{
					lambda_error(this, 0, 0, error_enum::bad_alloc, boost_error_code, "ThreadPoolTask:" + error);
				}
				else
				{
					if (error_enum_ == ThreadPoolTask__class::error_enum::other_error)
					{
						lambda_error(this, 0, 0, error_enum::other_error, boost_error_code, "ThreadPoolTask:" + error);
					}
				}

			});
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	}

	~BoostAsio__TCP_server()
	{
		stop();
	}



public:

	void wait_here()
	{
		try
		{
			if (ThreadType_enum == ThreadType_enum::multi_threads)
			{
				for (int i = 0; i < num_threads; i++)
				{
					if (vec_thread[i].joinable() == true)
					{
						//Значит у потока еще не вызывался .join().
						vec_thread[i].join();
					}
				}
			}
			else
			{
				io_context__Run();                    //Я гарантировано в блокирующем режиме дожидаюсь выполнения всех отмененных асинхронных задач, ДО уничтожения этого класса.
			}
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}

	}

	void stop()
	{

		ThreadPoolTask__uptr.get()->add_task([this]()
			{

				for (std::list<acceptor_struct>::iterator it = list_acceptor.begin(); it != list_acceptor.end(); ++it)
				{
					(*it).acceptor_.close();
					(*it).Sockets__class_uptr.get()->close_AllSocket();
				}

				work_guard_uptr.get()->reset();

			});

	}


	const std::string get__BoostErrorText(const int boost_ErrorCode)
	{

		if (boost_ErrorCode == boost::asio::error::access_denied)
		{
			return "access_denied";
		}
		if (boost_ErrorCode == boost::asio::error::address_family_not_supported)
		{
			return "address_family_not_supported";
		}
		if (boost_ErrorCode == boost::asio::error::address_in_use)
		{
			return "address_in_use";
		}
		if (boost_ErrorCode == boost::asio::error::already_connected)
		{
			return "already_connected";
		}
		if (boost_ErrorCode == boost::asio::error::already_open)
		{
			return "already_open";
		}
		if (boost_ErrorCode == boost::asio::error::already_started)
		{
			return "already_started";
		}
		if (boost_ErrorCode == boost::asio::error::bad_descriptor)
		{
			return "bad_descriptor";
		}
		if (boost_ErrorCode == boost::asio::error::broken_pipe)
		{
			return "broken_pipe";
		}
		if (boost_ErrorCode == boost::asio::error::connection_aborted)
		{
			return "connection_aborted";
		}
		if (boost_ErrorCode == boost::asio::error::connection_refused)
		{
			return "connection_refused";
		}
		if (boost_ErrorCode == boost::asio::error::connection_reset)
		{
			return "connection_reset";
		}
		if (boost_ErrorCode == boost::asio::error::eof)
		{
			return "eof";
		}
		if (boost_ErrorCode == boost::asio::error::fault)
		{
			return "fault";
		}
		if (boost_ErrorCode == boost::asio::error::fd_set_failure)
		{
			return "fd_set_failure";
		}
		if (boost_ErrorCode == boost::asio::error::host_not_found)
		{
			return "host_not_found";
		}
		if (boost_ErrorCode == boost::asio::error::host_not_found_try_again)
		{
			return "host_not_found_try_again";
		}
		if (boost_ErrorCode == boost::asio::error::host_unreachable)
		{
			return "host_unreachable";
		}
		if (boost_ErrorCode == boost::asio::error::interrupted)
		{
			return "interrupted";
		}
		if (boost_ErrorCode == boost::asio::error::invalid_argument)
		{
			return "invalid_argument";
		}
		if (boost_ErrorCode == boost::asio::error::in_progress)
		{
			return "in_progress";
		}
		if (boost_ErrorCode == boost::asio::error::message_size)
		{
			return "message_size";
		}
		if (boost_ErrorCode == boost::asio::error::name_too_long)
		{
			return "name_too_long";
		}
		if (boost_ErrorCode == boost::asio::error::network_down)
		{
			return "network_down";
		}
		if (boost_ErrorCode == boost::asio::error::network_reset)
		{
			return "network_reset";
		}
		if (boost_ErrorCode == boost::asio::error::network_unreachable)
		{
			return "network_unreachable";
		}
		if (boost_ErrorCode == boost::asio::error::not_connected)
		{
			return "not_connected";
		}
		if (boost_ErrorCode == boost::asio::error::not_found)
		{
			return "not_found";
		}
		if (boost_ErrorCode == boost::asio::error::not_socket)
		{
			return "not_socket";
		}
		if (boost_ErrorCode == boost::asio::error::no_buffer_space)
		{
			return "no_buffer_space";
		}
		if (boost_ErrorCode == boost::asio::error::no_data)
		{
			return "no_data";
		}
		if (boost_ErrorCode == boost::asio::error::no_descriptors)
		{
			return "no_descriptors";
		}
		if (boost_ErrorCode == boost::asio::error::no_memory)
		{
			return "no_memory";
		}
		if (boost_ErrorCode == boost::asio::error::no_permission)
		{
			return "no_permission";
		}
		if (boost_ErrorCode == boost::asio::error::no_protocol_option)
		{
			return "no_protocol_option";
		}
		if (boost_ErrorCode == boost::asio::error::no_recovery)
		{
			return "no_recovery";
		}
		if (boost_ErrorCode == boost::asio::error::no_such_device)
		{
			return "no_such_device";
		}
		if (boost_ErrorCode == boost::asio::error::operation_not_supported)
		{
			return "operation_not_supported";
		}
		if (boost_ErrorCode == boost::asio::error::operation_aborted)
		{
			return "operation_aborted";
		}
		if (boost_ErrorCode == boost::asio::error::service_not_found)
		{
			return "service_not_found";
		}
		if (boost_ErrorCode == boost::asio::error::shut_down)
		{
			return "shut_down";
		}
		if (boost_ErrorCode == boost::asio::error::socket_type_not_supported)
		{
			return "socket_type_not_supported";
		}
		if (boost_ErrorCode == boost::asio::error::timed_out)
		{
			return "timed_out";
		}
		if (boost_ErrorCode == boost::asio::error::try_again)
		{
			return "try_again";
		}
		if (boost_ErrorCode == boost::asio::error::would_block)
		{
			return "would_block";
		}


		return std::to_string(boost_ErrorCode);

	}


	acceptor_struct* get__Acceptor_by_Name(const std::string& Acceptor_Name)
	{

		std::lock_guard<std::mutex>locker(ThreadPoolTask__uptr.get()->get__mutex_ref());

		for (std::list<acceptor_struct>::iterator it = list_acceptor.begin(); it != list_acceptor.end(); ++it)
		{
			if (it->Acceptor_Name == Acceptor_Name)
			{
				return &(*it);
			}
		}

		return 0;
	}

public:

	acceptor_struct* add__NewAcceptor(const std::string& Acceptor_Name, boost::asio::ip::tcp IP_type, const int port)
	{

		try
		{

			std::lock_guard<std::mutex>locker(ThreadPoolTask__uptr.get()->get__mutex_ref());


			//----------------------------------Создаем и открываем Новый Аццептор:Начало----------------------------------------
			boost::system::error_code boost_error_code;


			boost::asio::ip::tcp::endpoint endpoint_(IP_type, port);

			boost::asio::ip::tcp::acceptor acceptor_(io_context, endpoint_);
			//----------------------------------Создаем и открываем Новый Аццептор:Конец----------------------------------------


			list_acceptor.emplace_back(Acceptor_Name, std::move(acceptor_), IP_type, std::move(endpoint_));  //Добавляем Аццептор в список.

			list_acceptor.back().Sockets__class_uptr.reset(new Sockets__class(&io_context, [this](Sockets__class* Sockets__class_p, Sockets__class::socket_struct* socket_struct_p, const error_enum error_enum_, const boost::system::error_code& boost_error, const std::string& text_error)
				{
					lambda_error(this, &list_acceptor.back(), socket_struct_p, error_enum_, boost_error, text_error);
				}));


			return &(list_acceptor.back());

		}
		catch (std::bad_alloc& e)
		{
			boost::system::error_code boost_error_code;  //Просто затычка для вызова "Lambda_error"

			lambda_error(this, 0, 0, error_enum::bad_alloc, boost_error_code, "add__new_acceptor:" + (std::string)e.what());

			return 0;
		}
		catch (std::exception& e)
		{
			boost::system::error_code boost_error_code;  //Просто затычка для вызова "ambda_error"

			lambda_error(this, 0, 0, error_enum::boost_error, boost_error_code, "add__new_acceptor:" + (std::string)e.what());

			return 0;
		}

	}

	void start__Acceptor_Read(acceptor_struct* acceptor_struct_p, const size_t buff_read_size)
	{

		ThreadPoolTask__uptr.get()->add_task([this, acceptor_struct_p, buff_read_size]()
			{
				acceptor_struct_p->Read_struct_.Recieve_SizeBuf = buff_read_size;

				start__Acceptor_Read_(acceptor_struct_p);
			});

	}
	void start__Acceptor_ReadUntill(acceptor_struct* acceptor_struct_p, const std::string& user_separator)
	{

		ThreadPoolTask__uptr.get()->add_task([this, acceptor_struct_p, user_separator]()
			{

				acceptor_struct_p->Read_struct_.read_untill_.user_separator = user_separator;


				//-----------------------------------------------------------------
				Sockets__class::socket_struct* socket_struct_p = acceptor_struct_p->Sockets__class_uptr.get()->open_Previously_ClosedSocket(acceptor_struct_p->IP_type);   //Получаем ранее закрытый сокет и открываем его.

				if (socket_struct_p == 0)
				{
					//Значит ранее Закрытых сокетах в очереди нет. Значит добавим новый Сокет.

					socket_struct_p = acceptor_struct_p->Sockets__class_uptr.get()->add__NewSocket(acceptor_struct_p->IP_type);
				}
				//-----------------------------------------------------------------


				//-----------------------------------------------------------------
				if (socket_struct_p != 0)
				{
					start__Acceptor_ReadUntill_(acceptor_struct_p);
				}
				//-----------------------------------------------------------------


			});

	}

	void set__CallbackMode_for_SocketWrite(acceptor_struct* acceptor_struct_p, const CallbackMode CallbackMode_)
	{
		ThreadPoolTask__uptr.get()->add_task([this, acceptor_struct_p, CallbackMode_]()
			{
				acceptor_struct_p->Write_struct_.CallbackMode_ = CallbackMode_;
			});
	}

public:

	void send_to_socket(acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, char* pointer_to_buff, const size_t buff_size, const bool data_copy_flag, std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_sender_p, acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, const CallbackMessage CallbackMessage_, const size_t bytes_send_transferred, const size_t bytes_send_transferred_TOTAL)> User_Callback_for_Send)
	{

		//-----------------------------------
		char* pointer_to_buff_ = 0;

		if (data_copy_flag == true)
		{
			//Так как при передаче данных с помощью "async_write_some" - требуется, чтобы исходный буффер сохранялся и не изменялся на все время передачи, то, чтобы Пользователь не держал буффер сохранным сделаем его копию - хотя конечно - это дополнительные затарты на копирование.

			socket_struct_p->Write_struct_.Data_copy.resize(0);
			socket_struct_p->Write_struct_.Data_copy.append(pointer_to_buff, buff_size);

			pointer_to_buff_ = &(socket_struct_p->Write_struct_.Data_copy)[0];
		}
		else
		{
			pointer_to_buff_ = pointer_to_buff;
		}
		//-----------------------------------



		ThreadPoolTask__uptr.get()->add_task([this, acceptor_struct_p, socket_struct_p, pointer_to_buff_, buff_size, User_Callback_for_Send]()
			{
				send_to_socket__(acceptor_struct_p, socket_struct_p, pointer_to_buff_, buff_size, User_Callback_for_Send);
			});
	}

	void close_socket(acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p)
	{
		acceptor_struct_p->get__SocketClass_pointer()->close_Socket(socket_struct_p);
	}

	void close_AllSocket(acceptor_struct* acceptor_struct_p)
	{
		acceptor_struct_p->get__SocketClass_pointer()->close_AllSocket();
	}
	void delete_AllSocket(acceptor_struct* acceptor_struct_p)
	{
		acceptor_struct_p->get__SocketClass_pointer()->delete_AllSocket();
	}



public:


	void set__Callback_Error(std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_server_p, acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, const error_enum error_enum_, const boost::system::error_code& boost_error, const std::string& error_string)> user_lambda_)
	{
		ThreadPoolTask__uptr.get()->add_task([this, user_lambda_]()
			{
				lambda_error = user_lambda_;
			});
	}

	void set__Callback_for_SocketRead(std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_server_p, acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, const char* pointer_to_DataReceived, const size_t bytes_received)> user_lambda_)
	{
		ThreadPoolTask__uptr.get()->add_task([this, user_lambda_]()
			{
				User_Callback_for_Read = user_lambda_;
			});
	}

	void set__Callback_for_Connect(std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_server_p, acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, const Connect_flag Connect_flag_, const boost::system::error_code& boost_error) > user_lambda_)
	{

		ThreadPoolTask__uptr.get()->add_task([this, user_lambda_]()
			{
				User_Callback_AsyncConnect = user_lambda_;
			});

	}

	void set__Callback_for_SocketWrite(std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_server_p, acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, const CallbackMessage CallbackMessage_, const size_t bytes_send_transferred, const size_t bytes_send_transferred_TOTAL)> user_lambda_)
	{

		ThreadPoolTask__uptr.get()->add_task([this, user_lambda_]()
			{
				User_Shared_Callback_for_Send = user_lambda_;
			});

	}


private:

	void start__Acceptor_Read_(acceptor_struct* acceptor_struct_p)
	{



		//-----------------------------------------------------------------
		Sockets__class::socket_struct* socket_struct_p = acceptor_struct_p->Sockets__class_uptr.get()->open_Previously_ClosedSocket(acceptor_struct_p->IP_type);   //Получаем ранее закрытый сокет и открываем его.

		if (socket_struct_p == 0)
		{
			//Значит ранее Закрытых сокетах в очереди нет. Значит добавим новый Сокет.

			socket_struct_p = acceptor_struct_p->Sockets__class_uptr.get()->add__NewSocket(acceptor_struct_p->IP_type);
		}

		socket_struct_p->Read_struct_.set__SizeBuff_for_Read(acceptor_struct_p->Read_struct_.Recieve_SizeBuf);     //Устанавливаем размер приемного буффера, которое передал Пользовтаель или который установлен по умолчанию.
		//-----------------------------------------------------------------





		acceptor_struct_p->acceptor_.async_accept(socket_struct_p->socket_, [this, acceptor_struct_p, socket_struct_p](const boost::system::error_code& boost_error_code)
			{

				ThreadPoolTask__uptr.get()->add_task([this, acceptor_struct_p, socket_struct_p, boost_error_code]()
					{


						if (boost_error_code.value() != boost::system::errc::success)
						{
							if (boost_error_code.value() != boost::asio::error::operation_aborted)        //operation_aborted - означает, что задача была отменена вызовом метода "cancel" у обьекта "resolver_uptr" - поэтому отфильтруем эти "ошибки" - так как на самом деле они не ошбики.
							{

								//--------------------------------------------------
								const bool res = CheckSocket_on_Disconnect(acceptor_struct_p, socket_struct_p, boost_error_code);    //Проверим соответвует ли пришедшая ошибка - разорванному соединению. Если возвращает "true" - то значит, эта "ошибка" соовтетвует закртию соединения и не нужно ее расмтаривать, как ошибку и вызывать "lambda_error".
								//--------------------------------------------------


								//--------------------------------------------------
								if (res == false)
								{
									lambda_error(this, acceptor_struct_p, socket_struct_p, error_enum::boost_error, boost_error_code, "start__Acceptor_Read_:async_accept:" + get__BoostErrorText(boost_error_code.value()) + ":" + boost_error_code.message());
								}
								//--------------------------------------------------
							}
						}
						else
						{
							//Значит пришел новый запрос на соединение от удаленного IP и ранее переданный сокет в "async_accept" - уже находится в подключенном состоянии. Поставим сокет на чтение.

							set__ConnectFlag_value(socket_struct_p, true);                 //Ставим флаг, что на Сокете установлено соединение.

							User_Callback_AsyncConnect(this, acceptor_struct_p, socket_struct_p, Connect_flag::Connect, boost_error_code);      //Вызываем колбек сообщая Пользовтаелю, что Новое соединенеи установлено.

							set__Read(acceptor_struct_p, socket_struct_p);                 //Запускаем чтение на сокете.

							start__Acceptor_Read_(acceptor_struct_p);                       //Запускаем опять аццептор, чтобы он принимал следующие соединения.
						}

					});

			});


	}

	void start__Acceptor_ReadUntill_(acceptor_struct* acceptor_struct_p)
	{



		//-----------------------------------------------------------------
		Sockets__class::socket_struct* socket_struct_p = acceptor_struct_p->Sockets__class_uptr.get()->open_Previously_ClosedSocket(acceptor_struct_p->IP_type);   //Получаем ранее закрытый сокет и открываем его.

		if (socket_struct_p == 0)
		{
			//Значит ранее Закрытых сокетах в очереди нет. Значит добавим новый Сокет.

			socket_struct_p = acceptor_struct_p->Sockets__class_uptr.get()->add__NewSocket(acceptor_struct_p->IP_type);
		}
		//-----------------------------------------------------------------



		acceptor_struct_p->acceptor_.async_accept(socket_struct_p->socket_, [this, acceptor_struct_p, socket_struct_p](const boost::system::error_code& boost_error_code)
			{


				ThreadPoolTask__uptr.get()->add_task([this, acceptor_struct_p, socket_struct_p, boost_error_code]()
					{


						if (boost_error_code.value() != boost::system::errc::success)
						{
							if (boost_error_code.value() != boost::asio::error::operation_aborted)        //operation_aborted - означает, что задача была отменена вызовом метода "cancel" у обьекта "resolver_uptr" - поэтому отфильтруем эти "ошибки" - так как на самом деле они не ошбики.
							{

								//--------------------------------------------------
								const bool res = CheckSocket_on_Disconnect(acceptor_struct_p, socket_struct_p, boost_error_code);    //Проверим соответвует ли пришедшая ошибка - разорванному соединению.
								//--------------------------------------------------


								//--------------------------------------------------
								if (res == false)
								{
									lambda_error(this, acceptor_struct_p, socket_struct_p, error_enum::boost_error, boost_error_code, "start__Acceptor_ReadUntill_:async_accept:" + get__BoostErrorText(boost_error_code.value()) + ":" + boost_error_code.message());
								}
								//--------------------------------------------------
							}
						}
						else
						{
							//Значит пришел новый запрос на соединение от удаленного IP м ранее переданный сокет в "async_accept" - уже находится в подключенном состоянии. Поставим сокет на чтение.

							set__ConnectFlag_value(socket_struct_p, true);                 //Ставим флаг, что на Сокете установлено соединение.

							User_Callback_AsyncConnect(this, acceptor_struct_p, socket_struct_p, Connect_flag::Connect, boost_error_code);      //Вызываем колбек сообщая Пользовтаелю, что Новое соединенеи установлено.

							set__ReadUntil(acceptor_struct_p, socket_struct_p);                 //Запускаем чтение на сокете.

							start__Acceptor_ReadUntill_(acceptor_struct_p);     //Запускаем опять аццептор, чтобы он принимал следующие соединения.
						}

					});

			});

	}

private:

	void set__Read(acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p)
	{
		AsyncRead_handler(acceptor_struct_p, socket_struct_p);
	}

	void set__ReadUntil(acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p)
	{
		AsyncReadUntill_handler(acceptor_struct_p, socket_struct_p);
	}

private:


	inline void AsyncRead_handler(acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p)
	{

		socket_struct_p->socket_.async_read_some(boost::asio::buffer(&socket_struct_p->Read_struct_.BufferReceiver[0], socket_struct_p->Read_struct_.BufferReceiver.size()), [this, acceptor_struct_p, socket_struct_p](const boost::system::error_code& boost_error_code, const std::size_t data_size)
			{
				//Данный лямбда кол-бек ВСЕГДА будет вызываться в том потоке в котором запущен данных обьект класса(при установке при создании класса параметра main_thread) или в нескольких отдельных внутренних потоках (при установке при создании класса параметра othre_threads) Boost Asio - отличных от любого Пользовательского потока.

				ThreadPoolTask__uptr.get()->add_task([this, acceptor_struct_p, socket_struct_p, boost_error_code, data_size]()
					{

						if (boost_error_code.value() != boost::system::errc::success)
						{
							if (boost_error_code.value() != boost::asio::error::operation_aborted)        //operation_aborted - означает, что задача была отменена вызовом метода "cancel" у обьекта "resolver_uptr" - поэтому отфильтруем эти "ошибки" - так как на самом деле они не ошбики.
							{
								//--------------------------------------------------
								const bool res = CheckSocket_on_Disconnect(acceptor_struct_p, socket_struct_p, boost_error_code);    //Проверим соответвует ли пришедшая ошибка - разорванному соединению. Если возвращает "true" - то значит, эта "ошибка" соовтетвует закртию соединения и не нужно ее расмтаривать, как ошибку и вызывать "lambda_error".
								//--------------------------------------------------


								//--------------------------------------------------
								if (res == false)
								{
									lambda_error(this, acceptor_struct_p, socket_struct_p, error_enum::boost_error, boost_error_code, "AsyncRead_handler:async_read_some:" + get__BoostErrorText(boost_error_code.value()) + ":" + boost_error_code.message());
								}
								//--------------------------------------------------

							}
						}
						else
						{
							//Значит на Сокет пришло сообщение от удаленного IP размером "data_size":

							User_Callback_for_Read(this, acceptor_struct_p, socket_struct_p, socket_struct_p->Read_struct_.BufferReceiver.c_str(), data_size);

							AsyncRead_handler(acceptor_struct_p, socket_struct_p);
						}

					});

			});

	}
	inline void AsyncReadUntill_handler(acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p)
	{

		boost::asio::async_read_until(socket_struct_p->socket_, boost::asio::dynamic_buffer(socket_struct_p->Read_struct_.read_untill_.string_buff), acceptor_struct_p->Read_struct_.read_untill_.user_separator.c_str(), [this, acceptor_struct_p, socket_struct_p](const boost::system::error_code& boost_error_code, const std::size_t data_size)
			{
				//Данный лямбда кол-бек ВСЕГДА будет вызываться в том потоке в котором запущен данных обьект класса(при установке при создании класса параметра main_thread) или в нескольких отдельных внутренних потоках (при установке при создании класса параметра othre_threads) Boost Asio - отличных от любого ПОльзовательского потока.


				ThreadPoolTask__uptr.get()->add_task([this, acceptor_struct_p, socket_struct_p, boost_error_code, data_size]()
					{


						if (boost_error_code.value() != boost::system::errc::success)
						{

							if (boost_error_code.value() != boost::asio::error::operation_aborted)        //operation_aborted - означает, что задача была отменена вызовом метода "cancel" у обьекта "resolver_uptr" - поэтому отфильтруем эти "ошибки" - так как на самом деле они не ошбики.
							{
								//--------------------------------------------------
								const bool res = CheckSocket_on_Disconnect(acceptor_struct_p, socket_struct_p, boost_error_code);    //Проверим соответвует ли пришедшая ошибка - разорванному соединению. Если возвращает "true" - то значит, эта "ошибка" соовтетвует закртию соединения и не нужно ее расмтаривать, как ошибку и вызывать "lambda_error".
								//--------------------------------------------------


								//--------------------------------------------------
								if (res == false)
								{
									lambda_error(this, acceptor_struct_p, socket_struct_p, error_enum::boost_error, boost_error_code, "set__Read:AsyncRead_handler:" + get__BoostErrorText(boost_error_code.value()) + ":" + boost_error_code.message());
								}
								//--------------------------------------------------
							}
						}
						else
						{
							//Значит на Сокет пришло сообщение от удаленного IP размером в котором был найдет Пользовтательский разделитель. Внимание: если в Приемном сообщении после разделителя есть другие данные, то они не записываются в Пользовательскй буффер с этим колбек вызовом, они будт записаны в пользовательский std::string при последующем колбек вызове.

							User_Callback_for_Read(this, acceptor_struct_p, socket_struct_p, (socket_struct_p->Read_struct_.read_untill_.string_buff).c_str(), data_size - acceptor_struct_p->Read_struct_.read_untill_.user_separator.size());

							//---------------------------------------------------------------------------
							socket_struct_p->Read_struct_.read_untill_.string_buff.erase(0, data_size);     //Стираем все что было с начала Пользовательского буффера std::string ДО найденного разделителя включительно. ПОЭТОМУ ЧТО если это не стреть повторно вызывать "AsyncReadUntill_handler", то Boost Asio будет анализировать этот std::string с самого начала на предмет поиска разделителя.
							//---------------------------------------------------------------------------

							AsyncReadUntill_handler(acceptor_struct_p, socket_struct_p);
						}


					});

			});

	}


private:

	//*******************************************************************************************************************************************************************************************************************************

	void send_to_socket_(acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, char* pointer_to_buff, const size_t buff_size, std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_sender_p, acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, const CallbackMessage CallbackMessage_, const size_t bytes_send_transferred, const size_t bytes_send_transferred_TOTAL)> User_Callback_for_Send)
	{

		//----------------------------------------------------------------------------------------------------------------------------------------------
		socket_struct_p->Write_struct_.Full_Transfered__flag = false;   //Ставим флаг false - перед началом нового запроса, чтобы другие запросы становились в очередь.

		socket_struct_p->Write_struct_.UserBuff_struct_.pointer_to_Buff = pointer_to_buff;
		socket_struct_p->Write_struct_.UserBuff_struct_.Buff_size = buff_size;

		socket_struct_p->Write_struct_.bytes_send_transferred = 0; //Обнуляем перед началом нового запроса.
		//----------------------------------------------------------------------------------------------------------------------------------------------

		AsyncWrite_handler(acceptor_struct_p, socket_struct_p, User_Callback_for_Send);

	}
	void send_to_socket__(acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, char* pointer_to_buff, const size_t buff_size, std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_sender_p, acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, const CallbackMessage CallbackMessage_, const size_t bytes_send_transferred, const size_t bytes_send_transferred_TOTAL)> User_Callback_for_Send)
	{
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if (socket_struct_p->Write_struct_.Full_Transfered__flag == false)
		{
			//Значит в данный момент какой то запрос отсылает данные.

			socket_struct_p->Write_struct_.deque_for_Sequential_Send.push_back([this, acceptor_struct_p, socket_struct_p, pointer_to_buff, buff_size, User_Callback_for_Send]()
				{
					//Добавим данный запрос в очередь на ожидание, пока все данные по действующему запросу не будут отправлены.

					send_to_socket_(acceptor_struct_p, socket_struct_p, pointer_to_buff, buff_size, User_Callback_for_Send);
				});

		}
		else
		{
			//Значит в данный момент нет запросов, которые бы не завершили отсылку данных. Поэтому просто вызываем функцию отправки данных.

			send_to_socket_(acceptor_struct_p, socket_struct_p, pointer_to_buff, buff_size, User_Callback_for_Send);
		}
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	}


	inline void AsyncWrite_handler(acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_sender_p, acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, const CallbackMessage CallbackMessage_, const size_t bytes_send_transferred, const size_t bytes_send_transferred_TOTAL)> User_Callback_for_Send)
	{


		boost::asio::mutable_buffer buffer_ = boost::asio::buffer(socket_struct_p->Write_struct_.UserBuff_struct_.pointer_to_Buff, socket_struct_p->Write_struct_.UserBuff_struct_.Buff_size - socket_struct_p->Write_struct_.bytes_send_transferred);  //Связываем Пользовательский указатель на буффер с Boost Asio, вторым параметром указываем раззмер в байтах по этому указателю сколько еще осталось передать.


		socket_struct_p->socket_.async_write_some(buffer_, [this, acceptor_struct_p, socket_struct_p, User_Callback_for_Send](const boost::system::error_code& boost_error, const std::size_t bytes_send_transferred)
			{
				//Данный лямбда кол-бек ВСЕГДА будет вызываться в том потоке в котором запущен данных обьект класса(при установке при создании класса параметра main_thread) или в нескольких отдельных внутренних потоках (при установке при создании класса параметра othre_threads) Boost Asio - отличных от любого ПОльзовательского потока.


				ThreadPoolTask__uptr.get()->add_task([this, acceptor_struct_p, socket_struct_p, boost_error, bytes_send_transferred, User_Callback_for_Send]()
					{



						if (boost_error.value() != boost::system::errc::success)
						{

							if (boost_error.value() != boost::asio::error::operation_aborted)        //operation_aborted - означает, что задача была отменена вызовом метода "cancel" у обьекта "resolver_uptr" - поэтому отфильтруем эти "ошибки" - так как на самом деле они не ошбики.
							{

								//--------------------------------------------------
								const bool res = CheckSocket_on_Disconnect(acceptor_struct_p, socket_struct_p, boost_error);    //Проверим соответвует ли пришедшая ошибка - разорванному соединению. Если возвращает "true" - то значит, эта "ошибка" соовтетвует закртию соединения и не нужно ее расмтаривать, как ошибку и вызывать "lambda_error".
								//--------------------------------------------------


								//--------------------------------------------------
								if (res == false)
								{
									lambda_error(this, acceptor_struct_p, socket_struct_p, error_enum::boost_error, boost_error, "AsyncWrite_handler:async_write_some:" + get__BoostErrorText(boost_error.value()) + ":" + boost_error.message());
								}
								//--------------------------------------------------
							}

						}
						else
						{
							//Значит пришла часть отправленных данных. Нужно проверить эта часть полная и совпадает с размером данных, которые отправлял Пользователь или еще нет:

							socket_struct_p->Write_struct_.bytes_send_transferred = socket_struct_p->Write_struct_.bytes_send_transferred + bytes_send_transferred;   //Суммируем отправленные части.


							//-----------------------------------------------------------------------------------------------------
							if (socket_struct_p->Write_struct_.bytes_send_transferred == socket_struct_p->Write_struct_.UserBuff_struct_.Buff_size)
							{
								//Значит эта часть была последней:

								//------------------------------------------------------
								if (User_Shared_Callback_for_Send != 0)
								{
									User_Shared_Callback_for_Send(this, acceptor_struct_p, socket_struct_p, CallbackMessage::Full, bytes_send_transferred, socket_struct_p->Write_struct_.bytes_send_transferred);
								}

								if (User_Callback_for_Send != 0)
								{
									User_Callback_for_Send(this, acceptor_struct_p, socket_struct_p, CallbackMessage::Full, bytes_send_transferred, socket_struct_p->Write_struct_.bytes_send_transferred);
								}
								//------------------------------------------------------



								//**********************************************************************************************
								socket_struct_p->Write_struct_.Full_Transfered__flag = true;                       //Ставим флаг, что по данному запросу - все данные переданы.

								if (socket_struct_p->Write_struct_.deque_for_Sequential_Send.size() != 0)
								{
									//Значит в очереди сокета есть ожидающие задачи:

									socket_struct_p->Write_struct_.deque_for_Sequential_Send.front()();                                //Запускаем первую задачу из очереди.

									socket_struct_p->Write_struct_.deque_for_Sequential_Send.pop_front();                              //Удаляем задачу из очереди.
								}
								else
								{
									return;     //Значит задач в очереди нет, выходим и ничего не делаем.
								}
								//**********************************************************************************************

							}
							else
							{
								//Значит эта часть была еще Не последней:


								if (acceptor_struct_p->Write_struct_.CallbackMode_ == CallbackMode::Call_AlwaysPart_Send)
								{
									if (User_Shared_Callback_for_Send != 0)
									{
										User_Shared_Callback_for_Send(this, acceptor_struct_p, socket_struct_p, CallbackMessage::Part, bytes_send_transferred, socket_struct_p->Write_struct_.bytes_send_transferred);
									}
								}


								AsyncWrite_handler(acceptor_struct_p, socket_struct_p, User_Callback_for_Send);        //Так как еще не все данные передалались, то вызываем функцию "AsyncWrite_handler" до тех пор, пока все данные не передадутся, так как функция "async_write_some" вызывает колбек толко один раз, поэтому нужно вручную вызывать.

							}
							//-----------------------------------------------------------------------------------------------------


						}

					});

			});


	}

	//*******************************************************************************************************************************************************************************************************************************



private:


	//----------------------------------------------------------
	boost::asio::io_context io_context;

	std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_uptr;   //Эта штука нужна для того, чтобы io_context.run() ен завершался, если в данный момент на нем нет ни каких задач - вообщем чтобы io_context, как бы крутился в бесконечном цикле - пока его не отменят.
	//----------------------------------------------------------


	//----------------------------------------------------------
	std::list<acceptor_struct>list_acceptor;
	//----------------------------------------------------------



	//----------------------------------------------------------
	std::vector<std::thread>vec_thread;

	int num_threads;

	ThreadType_enum ThreadType_enum;
	//----------------------------------------------------------

	//std::mutex mutex_;
	std::unique_ptr<ThreadPoolTask__class>ThreadPoolTask__uptr;


	//--------------------------------------------------------------
	std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_server_p, acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, const error_enum, const boost::system::error_code& boost_error, const std::string& text_error)>lambda_error;
	//--------------------------------------------------------------
	std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_server_p, acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, const char* pointer_to_DataReceived, const size_t bytes_received)> User_Callback_for_Read;
	//--------------------------------------------------------------
	std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_server_p, acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, const Connect_flag Connect_flag_, const boost::system::error_code& boost_error)> User_Callback_AsyncConnect;
	//--------------------------------------------------------------
	std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_server_p, acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, const CallbackMessage CallbackMessage_, const size_t bytes_send_transferred, const size_t bytes_send_transferred_TOTAL)> User_Shared_Callback_for_Send;
	//--------------------------------------------------------------


	//*********************************************************************************************************************************************************************************************************


	inline void io_context__Run()
	{
		io_context.run();
	}




	const bool CheckSocket_on_Disconnect(acceptor_struct* acceptor_struct_p, Sockets__class::socket_struct* socket_struct_p, const boost::system::error_code& boost_error)
	{

		//Вообщем это коды ошибок Boost Asio говорящие о рарзывае или отсутвии соединения на сокете. Вомхожно есть еще какие то дополнительные ошибки, я пока нашел именно эти.

		bool check_flag = false;

		std::cout << boost_error.message() << std::endl;
		//------------------------------------------------------------------------------
		if (boost_error.value() == boost::asio::error::connection_refused)
		{
			set__ConnectFlag_value(socket_struct_p, false);

			check_flag = true;

			goto jump;
		}
		else if (boost_error.value() == boost::asio::error::connection_reset)
		{
			set__ConnectFlag_value(socket_struct_p, false);

			check_flag = true;

			goto jump;
		}
		else if (boost_error.value() == boost::asio::error::not_connected)
		{
			set__ConnectFlag_value(socket_struct_p, false);

			check_flag = true;

			goto jump;
		}
		else if (boost_error.value() == boost::asio::error::connection_aborted)
		{
			set__ConnectFlag_value(socket_struct_p, false);

			check_flag = true;

			goto jump;
		}
		else if (boost_error.value() == boost::asio::error::eof)
		{
			set__ConnectFlag_value(socket_struct_p, false);

			check_flag = true;

			goto jump;
		}
		else if (boost_error.value() == boost::asio::error::broken_pipe)
		{
			set__ConnectFlag_value(socket_struct_p, false);

			check_flag = true;

			goto jump;
		}
		else if (boost_error.value() == boost::asio::error::host_unreachable)
		{
			set__ConnectFlag_value(socket_struct_p, false);

			check_flag = true;

			goto jump;
		}
		else if (boost_error.value() == boost::asio::error::network_unreachable)
		{
			set__ConnectFlag_value(socket_struct_p, false);

			check_flag = true;

			goto jump;
		}
		else if (boost_error.value() == boost::asio::error::network_down)
		{
			set__ConnectFlag_value(socket_struct_p, false);

			check_flag = true;

			goto jump;
		}
		else if (boost_error.value() == boost::asio::error::network_reset)
		{
			set__ConnectFlag_value(socket_struct_p, false);

			check_flag = true;

			goto jump;
		}
		else if (boost_error.value() == boost::asio::error::shut_down)
		{
			set__ConnectFlag_value(socket_struct_p, false);

			check_flag = true;

			goto jump;
		}
		//----------------------------------------------------------------------------



	jump:

		if (check_flag == true)
		{
			User_Callback_AsyncConnect(this, acceptor_struct_p, socket_struct_p, Connect_flag::Disconnect, boost_error);
		}

		return check_flag;

	}

	inline void set__ConnectFlag_value(Sockets__class::socket_struct* socket_struct_p, const bool value)
	{
		socket_struct_p->Connect_flag = value;
	}



};


