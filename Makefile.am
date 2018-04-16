instdir=/usr/local/lib
AM_CXXFLAGS = -O3 -std=c++14
ACLOCAL_AMFLAGS = -I m4 
pkginclude_HEADERS = include/actor/context.h include/private/controllerApi.h include/actor/commandMap.h include/actor/actor.h include/actor/commandExecutor.h include/private/errorReaction.h include/actor/errorReactionFactory.h include/actor/senderApi.h include/actor/rawData.h include/actor/state.h include/actor/types.h include/actor/actorRegistry.h include/actor/errorActionDispatcher.h
noinst_HEADERS = test/test.h include/private/actorCommand.h include/private/actorContext.h include/private/actorController.h include/private/actorStateMachine.h include/private/clientSocket.h include/private/internalCommands.h include/private/connection.h include/private/descriptorWait.h include/private/exception.h include/private/executor.h include/actor/link.h include/private/netAddr.h include/private/proxyClient.h include/private/proxyContainer.h include/private/proxyServer.h include/private/serverSocket.h include/private/sharedMap.h include/private/sharedQueue.h include/private/sharedVector.h include/private/supervisor.h include/private/types.h include/private/uniqueId.h 
inst_LIBRARIES = libactor.a
libactor_a_CPPFLAGS = -I$(top_srcdir)/include
libactor_a_SOURCES = src/senderApi.cpp src/actorCommand.cpp src/actorContext.cpp src/actorController.cpp src/actor.cpp src/actorRegistry.cpp src/actorStateMachine.cpp src/clientSocket.cpp src/commandExecutor.cpp src/connection.cpp src/errorReaction.cpp src/errorReactionFactory.cpp src/executor.cpp src/link.cpp src/proxyClient.cpp src/proxyContainer.cpp src/proxyServer.cpp src/rawData.cpp src/serverSocket.cpp src/supervisor.cpp src/uniqueId.cpp
libactor_a_CXXFLAGS=-O3 -std=c++14 


lib_LTLIBRARIES=libactor.la
libactor_la_CPPFLAGS = -I$(top_srcdir)/include
libactor_la_SOURCES = $(libactor_a_SOURCES) 

TESTS=actor_shared_lib_tests actor_static_lib_tests integration_shared_lib_tests integration_static_lib_tests
check_PROGRAMS = actor_shared_lib_tests actor_static_lib_tests integration_shared_lib_tests integration_static_lib_tests
actor_shared_lib_tests_LDFLAGS = -L$(top_srcdir)/.libs -lactor
actor_shared_lib_tests_CPPFLAGS = -I$(top_srcdir)/include
actor_shared_lib_tests_SOURCES = test/test.cpp test/actor_test.cpp
actor_static_lib_tests_LDFLAGS = $(top_srcdir)/libactor.a
actor_static_lib_tests_CPPFLAGS = -I$(top_srcdir)/include
actor_static_lib_tests_SOURCES = $(actor_shared_lib_tests_SOURCES)
integration_shared_lib_tests_LDFLAGS = -L$(top_srcdir)/.libs -lactor
integration_shared_lib_tests_CPPFLAGS = -I$(top_srcdir)/include
integration_shared_lib_tests_SOURCES = test/test.cpp test/integration_tests.cpp
integration_static_lib_tests_LDFLAGS = $(top_srcdir)/libactor.a
integration_static_lib_tests_CPPFLAGS = -I$(top_srcdir)/include
integration_static_lib_tests_SOURCES = $(integration_shared_lib_tests_SOURCES)
