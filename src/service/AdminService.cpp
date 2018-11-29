#include "service/AdminService.h"
#include "service/Application.h"
#include "service/ServiceCreator.h"

namespace mfw
{

void AdminService::initialize()
{
}

void AdminService::destroy()
{
}

void AdminService::shutdown()
{
    //g_app->terminate();
}

string AdminService::notify(const string &command)
{
	// TODO
    return command;
}

}
