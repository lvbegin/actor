ACLOCAL_AMFLAGS = -I m4 
include_HEADERS = include/actorStateMachine.h include/actorCommand.h include/actorContext.h include/actorController.h include/actor.h include/actorRegistry.h include/clientSocket.h include/commandExecutor.h include/commandValue.h include/connection.h include/descriptorWait.h include/errorStrategy.h include/exception.h include/executor.h include/linkApi.h include/messageQueue.h include/netAddr.h include/proxyClient.h include/proxyContainer.h include/proxyServer.h include/rawData.h include/serverSocket.h include/sharedMap.h include/sharedQueue.h include/sharedVector.h include/state.h include/supervisor.h include/types.h include/uniqueId.h
noinst_LIBRARIES = libactor.a
libactor_a_CPPFLAGS = -I$(top_srcdir)/include
libactor_a_SOURCES = src/actorController.cpp src/actor.cpp src/actorRegistry.cpp src/actorStateMachine.cpp src/clientSocket.cpp src/commandExecutor.cpp src/connection.cpp src/errorStrategy.cpp src/executor.cpp src/messageQueue.cpp src/proxyClient.cpp src/proxyContainer.cpp src/proxyServer.cpp src/serverSocket.cpp src/supervisor.cpp src/uniqueId.cpp
libactor_a_CXXFLAGS=-O3 


lib_LTLIBRARIES=libactor.la
libactor_la_CPPFLAGS = -I$(top_srcdir)/include
libactor_la_SOURCES = src/actorController.cpp src/actor.cpp src/actorRegistry.cpp src/actorStateMachine.cpp src/clientSocket.cpp src/commandExecutor.cpp src/connection.cpp src/errorStrategy.cpp src/executor.cpp src/messageQueue.cpp src/proxyClient.cpp src/proxyContainer.cpp src/proxyServer.cpp src/serverSocket.cpp src/supervisor.cpp src/uniqueId.cpp

TESTS=actor_shared_lib_tests actor_static_lib_tests
check_PROGRAMS = actor_shared_lib_tests actor_static_lib_tests
actor_shared_lib_tests_LDFLAGS = -L$(top_srcdir)/.libs -lactor
actor_shared_lib_tests_CPPFLAGS = -I$(top_srcdir)/include
actor_shared_lib_tests_SOURCES = test/actor_test.cpp
actor_static_lib_tests_LDFLAGS = -L$(top_srcdir) -lactor
actor_static_lib_tests_CPPFLAGS = -I$(top_srcdir)/include
actor_static_lib_tests_SOURCES = test/actor_test.cpp

