#include <iostream>
#include <string>
#include <vector>



#include "BoostAsio__TCP_server.h"




int main() 
{


	try
	{


		std::unique_ptr<BoostAsio__TCP_server>BoostAsio__TCP_server__utr;


		BoostAsio__TCP_server__utr.reset(new BoostAsio__TCP_server(BoostAsio__TCP_server::ThreadType_enum::main_thread, [](BoostAsio__TCP_server* BoostAsio__TCP_server_p, BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p, BoostAsio__TCP_server::Sockets__class::socket_struct* socket_struct_p, const  BoostAsio__TCP_server::error_enum error_enum_, const boost::system::error_code& boost_error, const std::string& error_string)
			{
				std::cout << "ERROR:" << error_string << std::endl;
			}));







		//**********************************************************************************1-BoostAsio__TCP_server:Начало*******************************************************************************************

		 //Класс "приема сообщений по TCP сокету":


		 //Конструктор класса принимает два параметра: 
		 //-main_thread   - значит io_context.run() будет вызван в данном потоке в котором создается обьект данного класса. Все ассинхронные колбеки будут вызыватся именно в этом данном потоке.
		 //-multi_threads - значит io_context.run() будет вызван в созданном пуле потоков по кол-во равному кол-ву физических потоков в системе. Все ассинхронные колбеки будут вызыватся именно в этом созданном пуле потков.
		 //И лямбду колбек, которая будет вызыватся в случае ошибок.
		 //ОЧЕНЬ ВАЖНО!!! Все Асинхронные функции или точнее их колбеки будут вызыватся именно так как описано в "main_thread" и "multi_threads", НО АБСОЛЮТНО ВСЕ ЗАДАЧИ ПОМЕЩАЮТСЯ В ПОТОКО-БЕЗОПАСНУЮ ОДНОПОТОЧНУЮ ОЧЕРЕДЬ И ВЫЗЫВАЮТСЯ ИЗ ПОТОКА ЭТОЙ ОЧЕРЕДИ - ТО ЕСТЬ СТРОГО ПОСЛЕДОВТАЕЛЬНО ИЗ ОДНОГО ПОТОКА.



		std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_server_p, BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p, BoostAsio__TCP_server::Sockets__class::socket_struct* socket_struct_p, const BoostAsio__TCP_server::error_enum error_enum_, const boost::system::error_code& boost_error_code, const std::string& error_string)>lamda_error_TCPSender = [](BoostAsio__TCP_server* BoostAsio__TCP_server_p, BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p, BoostAsio__TCP_server::Sockets__class::socket_struct* socket_struct_p, const BoostAsio__TCP_server::error_enum error_enum_, const boost::system::error_code& boost_error_code, const std::string& error_string)
			{

				//BoostAsio__TCP_server_p - указатель на обьект данного класса.
				//acceptor_struct_p       - указатель на аццептор, который принял данное Входящие соединение.
				//socket_struct_p         - указатель на обьект структры сокета.
				//error_enum_             - общий тип ошибки.
				//boost_error_code        - код ошибки boost
				//error_string            - текстовое наименование ошибки

				std::cout << "----------------------Error Callback: start-------------------------" << std::endl;

				std::cout << "Acceptor_Name Error:" << acceptor_struct_p->Acceptor_Name << std::endl;      //Имя сокета, на котором возникла ошибка.

				if (error_enum_ == BoostAsio__TCP_server::error_enum::boost_error)
				{
					std::cout << "That boost error:" << BoostAsio__TCP_server_p->get__BoostErrorText(boost_error_code.value()) << ":" << boost_error_code.message() << std::endl;
				}
				else
				{
					if (error_enum_ == BoostAsio__TCP_server::error_enum::bad_alloc)
					{
						std::cout << "That bad_alloc" << std::endl;
					}
				}

				std::cout << "error_string:" << error_string << std::endl;

				std::cout << "----------------------Error Callback: end-------------------------" << std::endl;
			};


		BoostAsio__TCP_server BoostAsio__TCP_server_(BoostAsio__TCP_server::ThreadType_enum::multi_threads, lamda_error_TCPSender);        //Создадим обьект класса.

		//Доступные методы:                     ВСЕ МЕТОДЫ ПОТОКО_БЕЗОПАСНЫ!!!!
		//- add__NewAcceptor                    - Добавляет новый Аццептор[прослушиватель]
		//- get__Acceptor_by_Name               - Возвращает структуру Аццептора по его имени указанном при вызове "get__Acceptor_by_Name".
		//- set__Callback_Error                 - Устанавливает общий колбек для всех Сокетов, который будет вызыватся при Ошибках. 
		//- set__Callback_for_SocketRead        - Устанавливает общий колбек для всех Сокетов, который будет вызыватся при Чтении данных их Сокета.
		//- set__Callback_for_Connect           - Устанавливает общий колбек для всех Сокетов, который будет вызыватся при изменеии статуса Соединения на Сокете.
		//- set__Callback_for_SocketWrite       - Устанавливает общий колбек для всех Сокетов, который будет вызыватся при Записи данных в Сокет.
		//- start__Acceptor_Read                - Запускает добалвенный ранее Аццептор на прослушивание.
		//- start__Acceptor_ReadUntill          - Запускает добалвенный ранее Аццептор на прослушивание в режиме "ReadUntill".
		//- set__CallbackMode_for_SocketWrite   - Устанавливает режим вызова колбека для метода "set__Callback_for_SocketWrite": будет ли "set__Callback_for_SocketWrite" вызыватся для каждой части отправленных данных в Сокет или будет вызыватся только когда все данные отправлемые данные помещены в Сокет. 
		//- send_to_socket                      - Отправляет данные в Сокет.
		//- close_socket                        - закрывает Сокет.
		//- close_AllSocket                     - закрывает все Сокеты.
		//- delete_AllSocket                    - открывает ранее закрытый Сокет.
		//- stop                                - останавливает работу класса удаляя и закрывая все атикные Сокеты и Соединения.
		//- wait_here                           - ждем окончания работы класса в этой точке.





		//------------------------------------------------------------------------------------add__NewAcceptor:Начало-------------------------------------------------------------------------------

		//Данный метод создает и добавляет новый Аццептор. Ацептор - это обьект, который принимает на указанном порту Входящие соединения с удаленным IP - создавая на каждое такое соединение новый сокет или используя ранее созданный сокет, но уже закрытый.
		//ДАННЫЙ МЕТОД ПОТОКО_БЕЗОПАСЕН.


		//-------------------------------------
		std::string Acceptor_name_1 = "Acceptor_Name_1";                          //Идентификатор Аццептора. НЕ ДОЛЖЕН ПОВТОРЯЮТСЯ С ДРУГИМИ ДОБАВЛЕННЫМИ АЦЦЕТОПРАМИ.

		boost::asio::ip::tcp IP_type_1 = boost::asio::ip::tcp::v4();              //Тип протокола TCP: IPv4 или IPv6.

		int port_1 = 1234;                                                        //Порт который прослушивает Аццетор и принимает соединения.
		//-------------------------------------

		BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p_ = BoostAsio__TCP_server_.add__NewAcceptor(Acceptor_name_1, IP_type_1, port_1);    //Возвращает указатель на структуру "акцептора".

		//------------------------------------------------------------------------------------add__NewAcceptor:Конец-------------------------------------------------------------------------------




		//---------------------------------------------------------------------------------get__Acceptor_by_Name:Начало---------------------------------------------------------------------------------

		//Возвращает указатель на акцептор по его имени.

		BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p = BoostAsio__TCP_server_.get__Acceptor_by_Name(Acceptor_name_1);

		if (acceptor_struct_p == 0)
		{
			//Значит такого имени не существует.
		}
		//---------------------------------------------------------------------------------get__Acceptor_by_Name:Конец---------------------------------------------------------------------------------



		//------------------------------------------------------------------------------------set__Callback_Error:Начало-------------------------------------------------------------------------------

		//Данные метод устанавилвает колбек ошибок, заменяя предыдущий установленный.
		//Данный метод потоко-безопасен.
		//Данный метод является обязательным к установке.

		/*
		//----------------------------------------------------
		std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_server_p, BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p, BoostAsio__TCP_server::Sockets__class::socket_struct* socket_struct_p, const BoostAsio__TCP_server::error_enum error_enum_, const boost::system::error_code& boost_error, const std::string& error_string)> user_lambda_2 = [](BoostAsio__TCP_server* BoostAsio__TCP_server_p, BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p, BoostAsio__TCP_server::Sockets__class::socket_struct* socket_struct_p, const BoostAsio__TCP_server::error_enum error_enum_, const boost::system::error_code& boost_error, const std::string& error_string)
			{
				//Данная лямбда будет вызыватся для всех сокетов из списка добавленных сокетов - на которых произошло данное событие.

				//BoostAsio__TCP_server_p - указатель на обьект данного класса.
				//acceptor_struct_p       - указатель на аццептор, который принял данное Входящие соединение.
				//socket_struct_p         - указатель на обьект структры сокета.
				//error_enum_             - общий тип ошибки.
				//boost_error_code        - код ошибки boost
				//error_string            - текстовое наименование ошибки

				std::cout << "----------------------Error Callback: start-------------------------" << std::endl;

				std::cout << "Acceptor_Name Error:" << acceptor_struct_p->Acceptor_Name << std::endl;      //Имя сокета, на котором возникла ошибка.

				if (error_enum_ == BoostAsio__TCP_server::error_enum::boost_error)
				{
					std::cout << "That boost error:" << BoostAsio__TCP_server_p->get__BoostErrorText(boost_error_code.value()) << ":" << boost_error_code.message() << std::endl;
				}
				else
				{
					if (error_enum_ == BoostAsio__TCP_server::error_enum::bad_alloc)
					{
						std::cout << "That bad_alloc" << std::endl;
					}
				}

				std::cout << "error_string:" << error_string << std::endl;

				std::cout << "----------------------Error Callback: end-------------------------" << std::endl;

			};
		//----------------------------------------------------


		BoostAsio__TCP_server_.set__Callback_Error(user_lambda_2);
		*/
		//------------------------------------------------------------------------------------set__Callback_Error:Конец-------------------------------------------------------------------------------




		//------------------------------------------------------------------------------------set__Callback_for_Connect:Начало-------------------------------------------------------------------------------

		//Данные метод устанавливает колбек Установки Соединения или Разрыва Соединения, заменяя предыдущий установленный.
		//Данный метод потоко-безопасен.
		//Данный метод является обязательным к установке.

		//----------------------------------------------------
		std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_server_p, BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p, BoostAsio__TCP_server::Sockets__class::socket_struct* socket_struct_p, const BoostAsio__TCP_server::Connect_flag Connect_flag_, const boost::system::error_code& boost_error)> user_lambda_3 = [](BoostAsio__TCP_server* BoostAsio__TCP_server_p, BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p, BoostAsio__TCP_server::Sockets__class::socket_struct* socket_struct_p, const BoostAsio__TCP_server::Connect_flag Connect_flag_, const boost::system::error_code& boost_error)
			{

				//Данная лямбда будет вызыватся для всех сокетов из списка добавленных сокетов - на которых произошло данное событие.

				//BoostAsio__TCP_server_p - указатель на обьект данного класса.
				//acceptor_struct_p       - указатель на аццептор, который принял данное Входящие соединение.
				//socket_struct_p         - указатель на обьект структры сокета.
				//Connect_flag_           - флаг соединения.


				std::cout << "------------------------------------Connect callback: start----------------------------------------------" << std::endl;

				if (Connect_flag_ == BoostAsio__TCP_server::Connect_flag::Connect)
				{
					std::cout << acceptor_struct_p->Acceptor_Name << ":New Connect IP adress:" << socket_struct_p->socket_.remote_endpoint().address().to_string() << socket_struct_p->socket_.remote_endpoint().port() << std::endl;  //Выводим IP с которым был установлено соеднение.
				}

				if (Connect_flag_ == BoostAsio__TCP_server::Connect_flag::Disconnect)
				{
					std::cout << acceptor_struct_p->Acceptor_Name << ": Disconnect:" << BoostAsio__TCP_server_p->get__BoostErrorText(boost_error.value()) << std::endl;
				}

				std::cout << "------------------------------------Connect callback: end------------------------------------------------" << std::endl;


			};
		//----------------------------------------------------


		BoostAsio__TCP_server_.set__Callback_for_Connect(user_lambda_3);


		//------------------------------------------------------------------------------------set__Callback_for_Connect:Конец-------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------set__Callback_for_SocketWrite:Начало-------------------------------------------------------------------------------

		//Данные метод устанавливает колбек информарования о том, сколько байт записано в сетевой драйвер для отправи сообщения, заменяя предыдущий установленный.
		//Данный метод потоко-безопасен.
		//Данный метод НЕ является обязательным к установке.

		//----------------------------------------------------
		std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_server_p, BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p, BoostAsio__TCP_server::Sockets__class::socket_struct* socket_struct_p, const BoostAsio__TCP_server::CallbackMessage CallbackMessage_, const size_t bytes_send_transferred, const size_t bytes_send_transferred_TOTAL)> user_lambda_4 = [](BoostAsio__TCP_server* BoostAsio__TCP_server_p, BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p, BoostAsio__TCP_server::Sockets__class::socket_struct* socket_struct_p, const BoostAsio__TCP_server::CallbackMessage CallbackMessage_, const size_t bytes_send_transferred, const size_t bytes_send_transferred_TOTAL)
			{
				//Данная лямбда будет вызыватся для всех сокетов из списка добавленных сокетов - на которых произошло данное событие.

				//BoostAsio__TCP_server_p         - указатель на обьект данного класса.
				//acceptor_struct_p               - указатель на аццептор, который принял данное Входящие соединение.
				//socket_struct_p                 - указатель на обьект структры сокета.
				//CallbackMessage_                - флаг: Part - значит это очередная часть передающегося сообщения, Full - значит это полсденяя часть переданного сообщения и сообщение считается полностью переданным.
				//bytes_send_transferred          - размер этой переданной части.
				//bytes_send_transferred_TOTAL    - размер суммарно переданных байт.


				std::cout << "------------------------------------Send callback: start----------------------------------------------" << std::endl;

				if (CallbackMessage_ == BoostAsio__TCP_server::CallbackMessage::Part)
				{
					std::cout << acceptor_struct_p->Acceptor_Name << ":" << bytes_send_transferred << " of " << bytes_send_transferred_TOTAL << ":Part. To IP:" << socket_struct_p->socket_.remote_endpoint().address().to_string() << std::endl;
				}

				if (CallbackMessage_ == BoostAsio__TCP_server::CallbackMessage::Full)
				{
					std::cout << acceptor_struct_p->Acceptor_Name << ":" << bytes_send_transferred << " of " << bytes_send_transferred_TOTAL << ":Full. To IP:" << socket_struct_p->socket_.remote_endpoint().address().to_string() << std::endl;
				
					//BoostAsio__TCP_server_p->close_socket(acceptor_struct_p, socket_struct_p);
				}

				std::cout << "------------------------------------Send callback: end------------------------------------------------" << std::endl;


			};
		//----------------------------------------------------


		BoostAsio__TCP_server_.set__Callback_for_SocketWrite(user_lambda_4);


		//------------------------------------------------------------------------------------set__Callback_for_SocketWrite:Конец-------------------------------------------------------------------------------


		//------------------------------------------------------------------------------------set__Callback_for_SocketRead:Начало-------------------------------------------------------------------------------

		//Данные метод добавляет колбек для приема данных на сокете, то есть он будет вызыватся каждый раз, как приходит какое то сообщение со стороны удаленного IP адресса.
		//Данный метод потоко-безопасен.
		//Данный метод является обязательным к установке.


		//----------------------------------------------------
		std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_server_p, BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p, BoostAsio__TCP_server::Sockets__class::socket_struct* socket_struct_p, const char* pointer_to_DataReceived, const size_t bytes_received)> user_lambda_5 = [](BoostAsio__TCP_server* BoostAsio__TCP_server_p, BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p, BoostAsio__TCP_server::Sockets__class::socket_struct* socket_struct_p, const char* pointer_to_DataReceived, const size_t bytes_received)
			{
				//Данная лямбда будет вызыватся для всех сокетов из списка добавленных сокетов - на которых произошло данное событие.


				//BoostAsio__TCP_server_p         - указатель на обьект данного класса.
				//acceptor_struct_p               - указатель на аццептор, который принял данное Входящие соединение.
				//socket_struct_p                 - указатель на обьект структры сокета.
				//pointer_to_DataReceived         - указатель на начало данных - куда записаны принятые данные.   Данные по указателю актуальны ТОЛЬКО в момент вызова данного колбека.
				//bytes_received                  - размер данных по указателю, который доступен для считывания.



				std::cout << "------------------------------------Read callback: start----------------------------------------------" << std::endl;

				std::string temp_string;

				temp_string.append(pointer_to_DataReceived, bytes_received);

				std::cout << acceptor_struct_p->Acceptor_Name << ": Recieve data from IP " << socket_struct_p->socket_.remote_endpoint().address().to_string() << ": " << bytes_received << " : " << temp_string << std::endl;

				std::cout << "------------------------------------Read callback: end------------------------------------------------" << std::endl;



				//``````````````````````````````````````````````````````
				static std::string string_to_send = "ECHO_Hello1_,_separator_ECHO_Hello2_,_separator_END";

				BoostAsio__TCP_server_p->send_to_socket(acceptor_struct_p, socket_struct_p, &string_to_send[0], string_to_send.size(), false, 0);    //Отправляем ответ на Входящее соббщение.
				//``````````````````````````````````````````````````````

			};
		//----------------------------------------------------


		BoostAsio__TCP_server_.set__Callback_for_SocketRead(user_lambda_5);


		//------------------------------------------------------------------------------------set__Callback_for_SocketRead:Конец-------------------------------------------------------------------------------




		//----------------------------------------------------------------------------------------start__Acceptor_Read:Начало-------------------------------------------------------------------------------------

		//Данный метод запускает ранее добавленный Аццептор на прослушивание Входящих соединений и чтения данных в случае принятия Входящих соединений и вызова Пользовательского колбека установенного в "set__Callback_for_SocketRead" на каждое принятое сообщение.
		//Запускается только один раз для какого то Аццептора. ЕСЛИ для конкретного Аццептора был запущен "start__Acceptor_ReadUntill" - то запускать данный метод уже нельзя!!!!!
		//Данный метод потоко-безопасен.


		//------------------------------------
		std::string Acceptor_name_6 = "Acceptor_Name_1";        //Имя ранее доавбленного Аццептора для которого запускаем данный метод.

		size_t buff_size_6 = 100;                      //Размер приемного буффера в котороый буде помещатся Пприемный сообщения, вообщем это условно разме Приемного чанка, котороыми будет читатся Приемные сообщения.
		//------------------------------------


		BoostAsio__TCP_server_.start__Acceptor_Read(BoostAsio__TCP_server_.get__Acceptor_by_Name(Acceptor_name_6), buff_size_6);

		//----------------------------------------------------------------------------------------start__Acceptor_Read:Конец-------------------------------------------------------------------------------------



		//-------------------------------------------------------------------------------------start__Acceptor_ReadUntill:Начало-------------------------------------------------------------------------------------

		//Данный метод запускает ранее добавленный Аццептор на прослушивание Входящих соединений и чтения данных в случае принятия Входящих соединений и вызова Пользовательского колбека установенного в "set__Callback_for_SocketRead" ТОЛЬКО в случае, когда во Входящем сообщении/сообщениях попадется заданная указанная подстрока.
		//Запускается только один раз для какого то Аццептора. ЕСЛИ для конкретного Аццептора был запущен "start__Acceptor_Read" - то запускать данный метод уже нельзя!!!!!
		//Данный метод потоко-безопасен.


		//------------------------------------
		std::string Acceptor_name_7 = "Acceptor_Name_1";        //Имя ранее доавбленного Аццептора для которого запускаем данный метод.

		std::string separator_7 = "fuck";                   //Подстрока, которая будет искатся в каждом Приемном сообщении и при нахождении вызыватся колбек установленный с помощью "set__Callback_for_SocketRead". ЕСЛИ на сокет поступило сообщение к примеру "1235_FUCK_6789", то вызовется колбек с указателем на начало сообщения и размером байт, который доступне по этому указателю, то есть 6 байт ДО подстроки "FUCK". Остальные байты "_6789" будут доступные только в слдеющий раз, когда опять встреится подстрока "FUCK", то есть остается в буффере "_6789" приходит следющее сообщение "Hello", теперь в буффере "_6789Hello" и в следующеи сообщении приходит "buyFUCK_3453", то есть в буффере "_6789HellobuyFUCK_3453" - опять вызывается колбек с указателем на начало строки и кол-ом байт дсотпных по указателю ровно ДО байта пред "FUCK", то есть "_6789Hellobuy" и так далее.
		//------------------------------------


		//BoostAsio__TCP_server_.start__Acceptor_ReadUntill(Acceptor_name_7, separator_7);

		//-------------------------------------------------------------------------------------start__Acceptor_ReadUntill:Конец-------------------------------------------------------------------------------------



		//-------------------------------------------------------------------------------------set__CallbackMode_for_SocketWrite:Начало-------------------------------------------------------------------------------------

		//Данный метод устанавливает флаг колбека для метода "send_to_socket": будет ли колбек вызыватся для всех частей передаваемого сообщения или вызовется только один раз, когда будет переданы все части сообщения.
		//Данный метод потоко-безопасен.


		//--------------------------------------------------
		std::string Acceptor_name_8 = "Acceptor_Name_1";                           //Идентификатор ранее добавленного сокета.

		BoostAsio__TCP_server::CallbackMode CallbackMode_8 = BoostAsio__TCP_server::CallbackMode::Call_AlwaysPart_Send;    //Call_AlwaysPart_Send - значит колбек будет вызыватся для все передаваемых частей. Call_OnlyFull_Send - Значит колбек вызовется только один раз для финальной части, когда все сообщение будет передано.
		//--------------------------------------------------


		BoostAsio__TCP_server_.set__CallbackMode_for_SocketWrite(BoostAsio__TCP_server_.get__Acceptor_by_Name(Acceptor_name_8), CallbackMode_8);

		//-------------------------------------------------------------------------------------set__CallbackMode_for_SocketWrite:Конец-------------------------------------------------------------------------------------




		//------------------------------------------------------------------------------------------send_to_socket:Начало----------------------------------------------------------------------------------------
		/*
		//Отправляет сообщение в Сокет по указанному акцептору.

		
		//-------------------------------------------------------------------------
		BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p;                        //Указатель на Ацкептор.
		BoostAsio__TCP_server::Sockets__class::socket_struct* socket_struct_p;            //Указатель на структуру Сокета. Ее можно получить при установки Соединения на Акцепторе из колбека "set__Callback_for_Connect"

		char* pointer_to_buff=0;                                                            //Указатель на данные, которые нужно отправить. 
		const size_t buff_size=0;                                                           //Размер данных по указателю, которые нужно отправить.
		
		bool data_copy_flag = false;                                                     //false - значит, что данные по указанному указателю "pointer_to_SendData_7" не будут скопированы во внутренний буффер класса и данные по указанному указателю нужно держать валидными на все время передачи данных; true - в этом случае данные по указанному указателю в размере "SendData_size_7" будут скопированы во внутренний буффер и с данными по указаннмо указателю можно делать все что угодно.

		std::function<void(BoostAsio__TCP_server* BoostAsio__TCP_sender__class_p, BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p, BoostAsio__TCP_server::Sockets__class::socket_struct* socket_struct_p, const BoostAsio__TCP_server::CallbackMessage CallbackMessage_, const size_t bytes_send_transferred, const size_t bytes_send_transferred_TOTAL)> user_lambda_send = [](BoostAsio__TCP_server* BoostAsio__TCP_sender__class_p, BoostAsio__TCP_server::acceptor_struct* acceptor_struct_p, BoostAsio__TCP_server::Sockets__class::socket_struct* socket_struct_p, const BoostAsio__TCP_server::CallbackMessage CallbackMessage_, const size_t bytes_send_transferred, const size_t bytes_send_transferred_TOTAL)
		{
			//Дання лямбда будет вызыватся только для конкретного Сокета - указанного при вызове "send_to_socket".


				//BoostAsio__TCP_server_p         - указатель на обьект данного класса.
				//acceptor_struct_p               - указатель на аццептор, который принял данное Входящие соединение.
				//socket_struct_p                 - указатель на обьект структры сокета.
				//CallbackMessage_                - флаг: Part - значит это очередная часть передающегося сообщения, Full - значит это полсденяя часть переданного сообщения и сообщение считается полностью переданным.
				//bytes_send_transferred          - размер этой переданной части.
				//bytes_send_transferred_TOTAL    - размер суммарно переданных байт.


				std::cout << "------------------------------------Send callback: start----------------------------------------------" << std::endl;

				if (CallbackMessage_ == BoostAsio__TCP_server::CallbackMessage::Part)
				{
					std::cout << acceptor_struct_p->Acceptor_Name << ":" << bytes_send_transferred << " of " << bytes_send_transferred_TOTAL << ":Part. To IP:" << socket_struct_p->socket_.remote_endpoint().address().to_string() << std::endl;
				}

				if (CallbackMessage_ == BoostAsio__TCP_server::CallbackMessage::Full)
				{
					std::cout << acceptor_struct_p->Acceptor_Name << ":" << bytes_send_transferred << " of " << bytes_send_transferred_TOTAL << ":Full. To IP:" << socket_struct_p->socket_.remote_endpoint().address().to_string() << std::endl;
				}

				std::cout << "------------------------------------Send callback: end------------------------------------------------" << std::endl;


		};
		//-------------------------------------------------------------------------

		 
		BoostAsio__TCP_server_.send_to_socket(acceptor_struct_p, socket_struct_p, pointer_to_buff, buff_size, data_copy_flag, user_lambda_send);              //ЕСЛИ КОЛБЕК НЕ НУЖЕН, ТО МОЖНО ПЕРЕДАТЬ - 0.
		*/
		//------------------------------------------------------------------------------------------send_to_socket:Конец----------------------------------------------------------------------------------------





	    //------------------------------------------------------------------------------------close_Socket:Начало-------------------------------------------------------------------------------
		/*
	    //Данный метод закрывает указанный сокет, отменяя все асинхроные операции и ВСЕ ЗАДАЧИ, который есть в очередях ожидания установки соединения и в очереди последоватльной отправки сообщений.
	    //ПОЭТОМУ если нужно закрыть Сокет, только после гарантированной отправки какого либо сообщения, то нужно использовать колбек вместе вызовом "send_to_socket" - и из колбекка уже вызывать закрытие сокета.

		std::string Acceptor_name_Close = "Acceptor_Name_1";                           //Идентификатор ранее добавленного сокета.

		BoostAsio__TCP_server::Sockets__class::socket_struct* Socket_p_close;                //Указатель на ранее добавленный сокет на указанном акцепторе. Сам сокет можно поулчить из колбека при вызове "set__Callback_for_Connect"

		BoostAsio__TCP_server_.close_socket(BoostAsio__TCP_server_.get__Acceptor_by_Name(Acceptor_name_Close), Socket_p_close);
		*/
		//------------------------------------------------------------------------------------close_Socket:Начало-------------------------------------------------------------------------------



		//------------------------------------------------------------------------------------close_AllSocket:Начало-------------------------------------------------------------------------------

		//Данный метод закрывает все сокеты, отменяя все асинхроные операции и ВСЕ ЗАДАЧИ, который есть в очередях ожидания установки соединения и в очереди последоватльной отправки сообщений.
		//Данный метод потоко-безопасен.


		//BoostAsio__TCP_sender_.get__SocketClass_pointer()->close_AllSocket();

		//------------------------------------------------------------------------------------close_AllSocket:Конец-------------------------------------------------------------------------------



		//------------------------------------------------------------------------------------delete_AllSocket:Начало-------------------------------------------------------------------------------

		//Данный метод просто удаляет все ранее созданные сокеты.
		//Данный метод потоко-безопасен.


		//BoostAsio__TCP_sender_.get__SocketClass_pointer()->delete_AllSocket();

		//------------------------------------------------------------------------------------delete_AllSocket:Конец-------------------------------------------------------------------------------







		//------------------------------------------------------------------------------------stop:Начало-------------------------------------------------------------------------------

		//Данный метод ВСЁ полностью останавливает:
		//-Останавливает все задачи "резолвы" на Сокете - если они есть.
		//-Останавливает все задади отсылки/приема на Сокете - если они есть.
		//-Дожидается в Блокирующем режиме завершения всех отмененых задач, которые выполнялись асинхронно. То есть пока все задачи не завершатся этот метод будет полностью блокировать дальнейшее выполненеи кода.
		//-Отменяет бесконечный цикл io_context, и код достигает вызова io_context.run() и проходит его До вызова "wait_here()" и далее. То есть теперь обьект данного класса не блокирует поток или потоки и Пользовательский код, который находится после "wait_here()" - будет выполнятся дальше, как обычно.
		//Данный метод потоко-безопасен.


		//BoostAsio__TCP_server_.stop();

		//Вообщем данный метод вызывается, когда нужно полностью завершить работу данного класса.

		//------------------------------------------------------------------------------------stop:Конец-------------------------------------------------------------------------------




		//------------------------------------------------------------------------------------wait_here:Начало-------------------------------------------------------------------------------

		//Данный метод - это аналог метода .join() для потоков. Данный класс будет крутится в условном бесконечом цикле не проходя метод "wait_here()" - пока класс полностью не оставят методом stop().
		//Данный метод всегда вызывается последним и ждет пока класс не завершат.
		//ВНИМАНИЕ!!!! УНИЧТОЖАТЬ ОБЬЕКТ ДАННОГО КЛАССА МОЖНО ТОЛЬКО ПОСЛЕ ТОГО, КАК ЗАВЕРШЕН ДАННЫЙ МЕТОД WAIT_HERE() - ПОТОМУ ЧТО ЛОГИЧНО ЕСЛИ КЛАСС УНИЧТОЖИТЬ РАНЬШЕ, ТО МЕТОД  wait_here() ВЫЗОВЕТСЯ У НУЛЕГО ОБЬЕКТА, ЧТО ПРИВЕДЕТ К НЕМЕДЛЕННОМУ ПАДЕНИЮ ПРОГРАММЫ.
		//Данный метод потоко-безопасен.


		BoostAsio__TCP_server_.wait_here();

		//------------------------------------------------------------------------------------wait_here:Конец-------------------------------------------------------------------------------


		 //**********************************************************************************2-BoostAsio__TCP_server:Конец*******************************************************************************************

	}
	catch (std::exception& e)
	{
		std::cout << "++++++++++exception:" << e.what() << std::endl;
	}


}
