#!/bin/bash

# The following environment variables are used:
# node_user     - A custom user to create
# node_password - The password for the user
# require_ssl   - Require SSL for all users except the replication user

# Wait until the node is ready

for ((i=0;i<100;i++))
do
    mysql -ss $1 -e 'show status like "wsrep_ready"' | grep 'ON' && break || sleep 1
done

mysql --force <<EOF

CREATE DATABASE IF NOT EXISTS test;

DROP USER IF EXISTS '$node_user'@'%';
CREATE USER '$node_user'@'%' IDENTIFIED BY '$node_password';
GRANT ALL ON *.* TO '$node_user'@'%' $require_ssl WITH GRANT OPTION;

DROP USER IF EXISTS 'repl'@'%';
CREATE USER 'repl'@'%' IDENTIFIED BY 'repl';
GRANT ALL ON *.* TO 'repl'@'%' WITH GRANT OPTION;

DROP USER IF EXISTS 'skysql'@'%';
CREATE USER 'skysql'@'%' IDENTIFIED BY 'skysql';
GRANT ALL ON *.* TO 'skysql'@'%' $require_ssl WITH GRANT OPTION;

DROP USER IF EXISTS 'maxskysql'@'%';
CREATE USER 'maxskysql'@'%' IDENTIFIED BY 'skysql';
GRANT ALL ON *.* TO 'maxskysql'@'%' $require_ssl WITH GRANT OPTION;

DROP USER IF EXISTS 'maxuser'@'%';
CREATE USER 'maxuser'@'%' IDENTIFIED BY 'maxpwd';
GRANT ALL ON *.* TO 'maxuser'@'%' $require_ssl WITH GRANT OPTION;

DROP DATABASE IF EXISTS test;
CREATE DATABASE test;

EOF
