/**
 * @file mx321.cpp regression case for bug MXS-321: https://mariadb.atlassian.net/browse/MXS-321
 * 
 *
 * - Set max_connections to 100
 * - Create 200 connections
 * - Close connections
 * - Check that maxadmin list servers shows 0 connections
 */

#include <my_config.h>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include "testconnections.h"
#include "maxadmin_operations.h"

using namespace std;

#define CONNECTIONS 200

void create_and_check_connections(TestConnections* test, int target)
{
    MYSQL* stmt[CONNECTIONS];

    for(int i=0;i<CONNECTIONS;i++)
    {
        switch(target)
        {
            case 1:
            stmt[i] = test->open_rwsplit_connection();
            break;

            case 2:
            stmt[i] = test->open_readconn_master_connection();
            break;

            case 3:
            stmt[i] = test->open_readconn_master_connection();
            break;
        }
    }

    for(int i=0;i<CONNECTIONS;i++)
    {
        if(stmt[i])
            mysql_close(stmt[i]);
    }

    sleep(10);

    char result[1024]; // = test->execute_ssh_maxscale((char*)"maxadmin list servers|grep 'server[0-9]'|cut -d '|' -f 4|tr -d ' '|uniq");
    int result_d;
    get_maxadmin_param(test->maxscale_IP, (char*) "admin", test->maxadmin_password, (char*) "maxadmin list servers", (char*) "Current no. of conns:", result);
    sscanf(result, "%d", &result_d);
    test->tprintf("result %s\t result_d %d\n", result, result_d);

    test->add_result(result_d, "Test failed: Expected 0 connections, but got %d", result_d);
}

int main(int argc, char *argv[])
{

    TestConnections * Test = new TestConnections(argc, argv);
    Test->set_timeout(50);

    Test->connect_maxscale();
    execute_query(Test->conn_rwsplit, "SET GLOBAL max_connections=100");
    Test->close_maxscale_connections();
    Test->stop_timeout();

    /** Create connections to readwritesplit */
    Test->set_timeout(50);
    create_and_check_connections(Test, 1);
    Test->stop_timeout();

    /** Create connections to readconnroute master */
    Test->set_timeout(50);
    create_and_check_connections(Test, 2);
    Test->stop_timeout();

    /** Create connections to readconnroute slave */
    Test->set_timeout(50);
    create_and_check_connections(Test, 3);
    Test->stop_timeout();

    Test->copy_all_logs();
    return(Test->global_result);
}
