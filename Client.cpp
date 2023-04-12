/**********************************************************************
 * <h1> Proyecto Chat </h3>
 * <h2> Cliente </h2>
 *
 * @author [Cristian Laynez, Sara Paguaga, Mario de Leon]
 *
 * <p> Universidad del Valle de Guatemala </p>
 * <p> Sistemas Operativos - Seccion 20 </p>
 *
 * ###################################################################
 * g++ Client.cpp proto/project.pb.cc -o Client.exe `pkg-config --cflags --libs protobuf` -lm
 * ./Client.exe nombreUsuario ipServidor puertoServidor
 * ./Client.exe usuarioNuevo 127.0.0.1 8080
 * ###################################################################
 **********************************************************************/

// Librerias para el uso del programa
#include <iostream>
#include <string>
#include <string.h>
#include <algorithm>
#include <cctype>
#include <stdlib.h>
// Importar librerias necesarias para sockets y threads
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ifaddrs.h>
// Para importar las estructuras del protocolo creado
#include "proto/project.pb.h"

using namespace std;

// Numeros constantes
#define BUFFER_SIZE 1024
int status, valread, client_fd;
char buffer[8080];

// ======================================================
// ===> Definir los metodos por si acaso
void mainApp(chat::UserRegister user);
void generalChat(chat::UserRegister user);
bool acceptedUser(char *userName);
bool is_str_empty(const string &str);
void *readMessages(void *args);

// ======================================================
// ===> Empezar a definir la logica de los metodos

void mainApp(chat::UserRegister user)
{
    system("clear");

    // Leer respueta del servidor para indicar que todo salio bien
    valread = read(client_fd, buffer, 1024);
    printf("\n---> %s\n\n", buffer);

    bool finish = false;
    while (!finish)
    {
        cout << "█ Bienvenid@: [" << user.username() << "]                                                           " << endl;
        cout << "█                                                                                                █  " << endl;
        cout << "█ 1) Chatear con todos los usuarios (broadcasting)                                               █  " << endl;
        cout << "█ 2) Chat individual                                                                             █  " << endl;
        cout << "█ 3) Cambiar Status                                                                              █  " << endl;
        cout << "█ 4) Ver Usuarios conectados en el chat                                                          █  " << endl;
        cout << "█ 5) Desplegar informacion de un usuario en particular                                           █  " << endl;
        cout << "█ 6) Ayuda                                                                                       █  " << endl;
        cout << "█ 7) Salir                                                                                       █  " << endl;
        cout << "█                                                                                                █  " << endl;
        cout << "██████████████████████████████████████████████████████████████████████████████████████████████████\n"
             << endl;

        string strtmp;
        cin >> strtmp;
        const char *tmp = strtmp.c_str();

        system("clear");

        if (strcmp(tmp, "1") == 0) // chat general / broadcasting
        {
            generalChat(user);
        }
        else if (strcmp(tmp, "2") == 0) // Chat individual
        {
        }
        else if (strcmp(tmp, "3") == 0) // Cambiar Status
        {
        }
        else if (strcmp(tmp, "4") == 0) // Usuarios conectados en el chat
        {
            // Solicitar el listado de usuarios
            chat::UserInfoRequest userInforequest;
            userInforequest.set_type_request(true);

            // Mandar los datos de la nueva request
            string requestSerialized;
            chat::UserRequest request;
            request.set_option(2);
            *request.mutable_newuser() = user;
            *request.mutable_inforequest() = userInforequest;
            request.SerializeToString(&requestSerialized); // Serializar informacion

            // Mandar la informacion al server
            strcpy(buffer, requestSerialized.c_str());
            send(client_fd, buffer, requestSerialized.size() + 1, 0);

            // Recibir la respuesta del servidor
            valread = read(client_fd, buffer, BUFFER_SIZE);

            // Deserializando el buffer recibido
            chat::ServerResponse server_response;
            server_response.ParseFromString(buffer);

            // Extrayendo informacion de los usuarios conectados de la respuesta del server
            chat::AllConnectedUsers connected_users = server_response.connectedusers();

            // Mostrando los usuarios conectados
            cout << "\nUsuarios conectados:\n\n";
            for (int i = 0; i < connected_users.connectedusers_size(); i++)
            {
                const chat::UserInfo &user_info = connected_users.connectedusers(i);
                cout << "Usario: " << user_info.username() << ", IP: " << user_info.ip() << endl;
            }
        }
        else if (strcmp(tmp, "5") == 0) // Desplegar informacion de un usuario en especial
        {
            chat::UserRequest userRequest;
            chat::InfoRequest *infoRequest = userRequest.mutable_inforequest();
            infoRequest->set_type_request(false);
            std::string input;
            std::cout << "Ingrese el username del usuario que desea buscar: ";
            std::getline(std::cin, input);
            infoRequest->set_input(input);

            // Serialize the user request and send it to the server
            std::string serialized_request;
            userRequest.SerializeToString(&serialized_request);
            send(server_socket, serialized_request.c_str(), serialized_request.length(), 0);

            // Wait for the server to respond
            char buffer[BUFFER_SIZE] = {0};
            int bytesReceived = recv(server_socket, buffer, BUFFER_SIZE, 0);
            if (bytesReceived == -1)
            {
                std::cerr << "Error en recv()" << std::endl;
                return 1;
            }

            // Deserialize the server response
            chat::ServerResponse serverResponse;
            serverResponse.ParseFromString(buffer);

            if (serverResponse.code() == 200)
            {
                if (serverResponse.option() == 2)
                {
                    std::cout << serverResponse.servermessage() << std::endl;

                    if (serverResponse.has_userinfo())
                    {
                        const chat::UserInfo &user = serverResponse.userinfo();
                        std::cout << "Username: " << user.username() << std::endl;
                        std::cout << "IP address: " << user.ip() << std::endl;
                        std::cout << "Status: " << user.status() << std::endl;
                    }
                    else
                    {
                        std::cout << "Usuario no encontrado." << std::endl;
                    }
                }
                else
                {
                    std::cout << "Opcion invalida del servidor." << std::endl;
                }
            }
            else
            {
                std::cout << "Error en el servidor: " << serverResponse.servermessage() << std::endl;
            }
        }
        else if (strcmp(tmp, "6") == 0) // Ayuda
        {
        }
        else if (strcmp(tmp, "7") == 0) // Salir
        {
            finish = true;
        }
        else // opcion invalida
        {
            cout << "\n----> OPCION INVALIDA\n"
                 << endl;
        }
    }
}

void generalChat(chat::UserRegister user)
{
    system("clear");
    // ---> Definir user info request

    // ---> Tener un ciclo while para representar el chat y obtener todos los mensajes
    bool outGeneralChat = false;
    while (!outGeneralChat)
    {
        cout << "\n====================================================================" << endl;
        cout << "Chat general: " << endl;
        // pthread_t read_thread;
        // if(pthread_create(&read_thread, NULL, readMessages, NULL) != 0)
        // {
        //     cout << "Error a la hora de crear pthreads" << endl;
        //     exit(EXIT_FAILURE);
        // }

        // Ofrecerle al usuario la posibilidad de escribir un mensaje o de regresar al menu principal
        cout << "\nEscribe un mensaje aqui (o puedes escribir [-regresar] si deseas regresar al menu principal)" << endl;
        // =======================================================
        string lineMessage;
        getline(cin, lineMessage);
        const char *message = lineMessage.c_str(); // Para realizar verificaciones

        // Verificar si la opcion solicitada es regresar
        if (strcmp(message, "-regresar") == 0)
        {
            outGeneralChat = true;
        }
        else if (is_str_empty(lineMessage)) // Para evitar que se ingrese mensajes vacios
        {
            cout << "El mensaje que trataste de enviar esta vacio -_-" << endl;
        }
        else // Si todo esta en orden se procede a mandar el correspondiente mensaje
        {
            // Crear el mensaje
            chat::newMessage newMessage;
            newMessage.set_message_type(true);      // Para definir que es chat general
            newMessage.set_sender(user.username()); // Ingrear el nombre del user
            newMessage.set_message(lineMessage);    // Ingresar el contenido del mensaje

            // Mandar el mensaje al servidor
            // ...

            // Revisar si el mensaje se envio de forma exitosa
            // ...

            // Notificar si asi fue el caso o no
            cout << "El mensaje fue enviado" << endl;
            cout << newMessage.message() << endl; // ! Esto esta por fines de verificacion
        }
    }
    system("clear");
}

void *readMessages(void *args)
{
    char buffer[BUFFER_SIZE];

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesRead = read(client_fd, buffer, BUFFER_SIZE);
        if (bytesRead <= 0)
        {
            cout << "Error a la hora de leer el mensaje" << endl;
            exit(EXIT_FAILURE);
        }

        // Mostar los mensajes recibidos
        cout << buffer << endl;
    }

    return NULL;
}

bool is_str_empty(const string &str)
{
    return all_of(str.begin(), str.end(), [](char c)
                  { return isspace(static_cast<unsigned char>(c)); });
}

int main(int argc, char *argv[])
{
    // Antes que nada vamos a ver si lo argumentos son validos
    int cantidad = argc;

    // Revisar que este la cantidad de argumentos requerido
    if (cantidad != 4)
    {
        cout << "--> FALTAN ARGUMENTOS PARA EJECUTAR EL **CLIENTE**, PORFAVOR SEGUIR LA SIGUIENTE ESTRUCUTRA:" << endl;
        cout << "    ./Client.exe nombreUsuario ipServidor puertoServidor" << endl;
        exit(1);
    }

    // De primero vamos a visualizar si la informacion solicitada es valida
    char *userName = argv[1];
    char *serverIP = argv[2];
    int serverPort = atoi(argv[3]);

    // Vamos a preparar todo lo escencial para la conexion
    struct sockaddr_in serv_addr;

    // Vamos a crear el socket correspondiente
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd <= 0)
    {
        cout << "Error en la creacion de socket :(" << endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serverPort);

    // Convertir IPv4 y IPv6 desde texto hasta binario
    if (inet_pton(AF_INET, serverIP, &serv_addr.sin_addr) <= 0)
    {
        cout << "Direccion IP Invalida :(" << endl;
        return -1;
    }

    // ==> Realizar la coneccion al server
    if ((status = connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        cout << "La conexion a fallado :(" << endl;
        return -1;
    }

    // Mandar informacion de un nuevo usuario registrado
    chat::UserRegister user;
    user.set_username(userName);
    user.set_ip(serverIP);

    // Mandar todos los datos necesarios al servidor
    string requestSerialized;
    chat::UserRequest userRequest;
    userRequest.set_option(1);             // 1 - Registro de Usuario
    *userRequest.mutable_newuser() = user; // UserRegister
    userRequest.SerializeToString(&requestSerialized);

    // Vamos a pasar la request de forma serializada
    strcpy(buffer, requestSerialized.c_str());
    send(client_fd, buffer, requestSerialized.size() + 1, 0);

    // Si todo esta en orden entonces ya se podra empezar el chat :)
    mainApp(user);

    close(client_fd);
    return 0;
}
