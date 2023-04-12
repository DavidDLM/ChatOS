/**
 * <h1> Proyecto Chat </h3>
 * <h2> Servidor </h2>
 *
 * @author [Cristian Laynez, Sara Paguaga, Mario de Leon]
 *
 * <p> Universidad del Valle de Guatemala </p>
 * <p> Sistemas Operativos - Seccion 20 </p>
 *
 * ###################################################################
 * Para instalar el protocolo:
 * protoc --cpp_out=. proto/project.proto
 * ###################################################################
 * g++ Server.cpp proto/project.pb.cc -o Server.exe `pkg-config --cflags --libs protobuf` -lm
 * ./Server.exe puertoServidor
 * ./Server.exe 8080
 * ###################################################################
 */

// Librerias normales
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
// Librerias para utilizar sockets y threads
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
// Importar las structs del protocolo propuesto
#include "proto/project.pb.h"

using namespace std;

int server_fd, new_socket, valread;
const char *welcome = "Entraste al chat :)";

struct ConnectionIP // Datos individuales por cada user
{
    int socketN;
    char ipAddress[INET_ADDRSTRLEN];
    char buffer[8080];
};

chat::AllConnectedUsers connectedUsers; // Almacenar todos los usuarios conectados

void *clientManager(void *arg)
{
    struct ConnectionIP *newConnection = (struct ConnectionIP *)arg;
    int socketNum = newConnection->socketN;
    char *ipAddress = newConnection->ipAddress;
    char *buffer = newConnection->buffer;

    chat::UserRequest userRequest;
    userRequest.ParseFromString(buffer); // Obtener Request

    // Obtener usuario
    chat::UserRegister newUser = userRequest.newuser();
    cout << "\nRevisar option: " << userRequest.option() << endl;
    cout << "\nUsuario conectado: " << newUser.username() << endl;

    // Vamos a verificar si el usuario existe
    chat::UserInfo userInformation;
    userInformation.set_username(newUser.username());
    userInformation.set_ip(ipAddress);
    userInformation.set_status(1); // Activo -> 1 // Ocupado -> 2 // Inactivo -> 3

    // Agregar la informacion de usuario a "connectedUsers"
    chat::UserInfo *adduserInformation = connectedUsers.add_connectedusers();
    adduserInformation->CopyFrom(userInformation);

    // // Enviar mensaje de acceso y confirmacion al usuario
    // valread = read(new_socket, buffer, 1024);
    // send(new_socket, welcome, strlen(welcome), 0);

    // Se llevara a cabo las diversas acciones que pueden ocurrir
    while (1)
    {
        // * ===> Vamos a verificar si el cliente tiene abierta la sesion
        if (recv(socketNum, buffer, 8080, 0) < 1)
        {
            if (recv(socketNum, buffer, 8080, 0) == 0)
            {
                cout << "\nNO CHILERISIMO >:V: [" << newUser.username() << "] se salio de este hermoso servidor" << endl;
            }
            break;
        }

        userRequest.ParseFromString(buffer); // Actualizar Request

        // * ===> Verificar las opciones que fueron seleccionadas
        if (userRequest.option() == 1) // ? Registro de usuario
        {
            cout << "Registro de usuario" << endl;
        }

        if (userRequest.option() == 2) // ? Solicitud de informacion de usuario // Opcion 4 y 5
        {
            // Se utlizara "UserInfoRequest" para determinar el tipo de informacion a pedir
            // ! ==> Si "type_request" es **TRUE** entonces se solicita la lista de TODOS LOS USUARIOS
            if (userRequest.inforequest().type_request() == true)
            {

                chat::ServerResponse server_response;

                // Estableciendo los campos requeridos
                server_response.set_option(2);
                server_response.set_code(200);
                server_response.set_servermessage("Muestra de usuarios conectados");

                *server_response.mutable_connectedusers() = connectedUsers;

                // Serializando respuesta
                string server_response_serialized;
                server_response.SerializeToString(&server_response_serialized);

                // Enviando respuesta serializada al cliente
                strcpy(buffer, server_response_serialized.c_str());
                send(socketNum, buffer, server_response_serialized.size() + 1, 0);
                // Mostrando usuarios conectados en el server
                cout << "Listado" << endl;
                for (int i = 0; i < connectedUsers.connectedusers_size(); ++i)
                {
                    const chat::UserInfo &user = connectedUsers.connectedusers(i);
                    cout << "Usuario: " << user.username() << ", IP: " << user.ip() << ", Status: " << user.status() << endl;
                }

                std::string serialized_users;
                connectedUsers.SerializeToString(&serialized_users);
                send(socketNum, serialized_users.c_str(), serialized_users.size(), 0);
            }
            // ! ==> Si "type_request" es **FALSE** entonces se solicita la informacion de SOLO UN USUARIO
            if (userRequest.inforequest().type_request() == false)
            {
                const std::string &input = userRequest.inforequest().input();
                const User *user = find_user_by_username(input);
                chat::ServerResponse response;
                response.set_option(2);
                response.set_code(200);

                if (user != nullptr)
                {
                    chat::UserInfo *userInfo = response.mutable_userinfo();
                    userInfo->set_username(user->username);
                    userInfo->set_ip(user->ip_address);
                    userInfo->set_status(user->status);
                }

                // Serialize the response and send it back to the client
                std::string serialized_response;
                response.SerializeToString(&serialized_response);
                send(client_socket, serialized_response.c_str(), serialized_response.length(), 0);
            }
        }

        if (userRequest.option() == 3) // ? Solicitud de cambio de status // Opcion 3
        {
        }

        if (userRequest.option() == 4) // ? Envio de nuevo mensaje // Opcion 2
        {
        }

        if (userRequest.option() == 5) // ? Heartbet (broadcasting) // Opcion 1
        {
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    // Antes que nada vamos a ver si lo argumentos son validos
    int cantidad = argc;

    // Revisar que este la cantidad de argumentos requerido
    if (cantidad != 2)
    {
        cout << "--> FALTAN ARGUMENTOS PARA EJECUTAR EL **SERVER**, PORFAVOR SEGUIR LA SIGUIENTE ESTRUCUTRA:" << endl;
        cout << "    ./Server.exe puertoServidor" << endl;
        exit(1);
    }

    int portNum = atoi(argv[1]); // Almacenar el port del arg

    // TODO: ==> Vamos a definir los elementos principales para la conexion
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Crear socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cout << "Socket fallido :(" << endl;
        exit(EXIT_FAILURE);
    }

    // Unir forzosamente el socket con el puerto impuesto...
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        cout << "Set Sockopt Error :(" << endl;
        exit(EXIT_FAILURE);
    }

    // Estas almacenan informacion sobre una direccion de socket...
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(portNum);

    // Unir forzosamente  de nuevo el socket con el puerto impuesto...
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    cout << "Esperando usuarios...\n"
         << endl;
    while (1)
    {
        // Esperar a que ingrese un nuevo usuario
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0)
        {
            cout << "Error :(" << endl;
            exit(EXIT_FAILURE);
        }

        // Agregar el nuevo cliente a la conexion con multithread
        struct ConnectionIP newConnection;
        newConnection.socketN = new_socket;
        inet_ntop(AF_INET, &(address.sin_addr), newConnection.ipAddress, INET_ADDRSTRLEN);

        // Enviar mensaje de acceso y confirmacion al usuario
        // valread = read(new_socket, buffer, 1024);
        valread = read(new_socket, newConnection.buffer, 1024);
        send(new_socket, welcome, strlen(welcome), 0);

        // Crear threads para agregar nuevas conexiones
        pthread_t client_thread;
        pthread_attr_t attrs;
        pthread_attr_init(&attrs);
        if (pthread_create(&client_thread, &attrs, clientManager, &newConnection))
        {
            cout << "Error a la hora de crear un pthread :(" << endl;
            exit(EXIT_FAILURE);
        }
    }

    // Cerrar todos los sockets creados
    close(new_socket);
    shutdown(server_fd, SHUT_RDWR);
    return 0;
}
