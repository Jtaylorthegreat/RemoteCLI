

# RemoteCLI

 Remote-CLI provides a web management portal to execute command line commands and remotely transfer scripts to your cloud servers.  Remote-CLI utilizes Openssl v1.1.1, spdlog, mysql c++ connector, and Andrew Schwartzmeyer's algorithm for breaking up files all of which can be found at the following:
 
 https://github.com/openssl/openssl
 
 https://github.com/gabime/spdlog
 
 https://github.com/mysql/mysql-connector-cpp
 
 https://gist.github.com/andschwa/d120be76ba8ebd66cf50
 
 

<br><br><br>





# Installation Guide:

## **Net-Controller (server daemon)**

<br>

**Centos 8:**
	
Add mysql community repository:
	
	rpm -Uvh http://repo.mysql.com/mysql80-community-release-el8.rpm
Install required packages:
	
	yum install mysql-connector-c++-devel httpd php php-mysqli mysql mysql-server

Start/enable mysqld service and setup mysql-server:
	
	systemctl start mysqld; systemctl enable mysqld
	mysql_secure_installation
Create user & database for Net-Controller:

    mysql -u root -p
    CREATE USER 'DBUSER'@'localhost' IDENTIFIED BY 'ENTERDBPASSWORDHERE'; 
    create database ENTERDATABASENAME; 
    grant all privileges on ENTERDATABASENAME.* to 'DBUSER'@'localhost'; 
    flush privileges;
Add Firewall rules:

	firewall-cmd --permanent --add-port=60130/tcp
	firewall-cmd --permanent --add-service=http
	firewall-cmd --reload
Install Net-Controller RPM:

	rpm -i Net-Controller-0.1-1.el8.x86_64.rpm

Run netcon-setup to configure tables for Net-Controller:

	cd /usr/sbin/ && exec ./netcon-setup DBNAME DBUSER DBPASS &

	add DB settings to:
	/opt/net-controller/net-controllerd/.serverconfig
	/var/www/controller-confs/config.ini
Enable & start Net-Controller Service:

	systemctl enable net-controller
	systemctl start net-controller


**Ubuntu 18.04:**

Install required packages:

	apt install php-mysql php mysql-server apache2 libmysqlcppconn-dev
	
Start/enable mysqld service and setup mysql-server:

	mysql_secure_installation
	systemctl enable mysql
Create user & database for Net-Controller:
	
	mysql -u root -p 
	CREATE USER 'DBUSER'@'localhost' IDENTIFIED BY 'DATABASEPWHERE!'; 
	create database ENTERDATABASENAME; 
	grant all privileges on ENTERDATABASENAME.* to 'DBUSER'@'localhost'; 
	flush privileges;
Install Net-Controller deb:

	dpkg -i net-controller-0.1-0.deb
Update "agentregistrationkey" for Net-Controller:	

	vi /opt/net-controller/net-controllerd/.serverconfig  

Enable & start Net-Controller Service:

	systemctl enable net-controller
	systemctl start net-controller
Add Firewall Rules:

	ufw allow http
	ufw allow 60130/tcp


<hr>

## Net-Agent Installation (client daemon)

**Centos 7:** 

Install Net-Agent rpm:

	rpm -i Net-Agent-0.1-1.el7.x86_64.rpm
Update "serveraddress" in agent configuration to reflect the appropriate controller:
	
	cd /opt/net-agent/net-agentd
	vi .agentconfig 
Register to the controller using the server's configured registration key:

	./net-agent -r SERVERREGKEY
	./net-agent -e SERVERREGKEY
Enable & start the net-agentd daemon:

	systemctl enable net-agentd
	systemctl start net-agentd

**Centos 8:**

Install Net-Agent rpm:

	rpm -i Net-Agent-0.1-1.el8.x86_64.rpm
	Update "serveraddress" in agent configuration to reflect the appropriate controller:
	
	cd /opt/net-agent/net-agentd
	vi .agentconfig 
Register to the controller using the server's configured registration key:

	./net-agent -r SERVERREGKEY
	./net-agent -e SERVERREGKEY
Enable & start the net-agentd daemon:

	systemctl enable net-agentd
	systemctl start net-agentd

**Ubuntu 18.04:**

Install Net-Agent deb:

	dpkg -i Net-Agent-0.1-0.Ubuntu18.04-x86.deb
Update "serveraddress" in agent configuration to reflect the appropriate controller:
	
	cd /opt/net-agent/net-agentd
	vi .agentconfig 
Register to the controller using the server's configured registration key:

	./net-agent -r SERVERREGKEY
	./net-agent -e SERVERREGKEY
Enable & start the net-agentd daemon:

	systemctl enable net-agentd
	systemctl start net-agentd
	
	
<hr>
<br><br><br>

## Tips/Caveats/Bugs:


   

 1. Due to openssl's ssl_write/ssl_read buffer being limited to 16384, script transfers utilize Andrew Schwartzmeyer's Algorithm to break up files before transmission.
 2. Currently files bigger than 2.14 Gb or 2147483647 bytes are too large for transfer due to INT_MAX limitation.
 3. Due to a coding issue, the "clientkey" entry in .agentconfig must remain at the bottom of the agent configuration file in order for the registration process to properly function.
 4. Net-Agents logs to /var/log/net-agentd/net-agentd.log and the net-agentd configuration file can be found at: */opt/net-agent/net-agentd/.agentconfig*
 5. Net-Controller logs to /var/log/net-controllerd/net-controllerd.log and the net-controller configuration file can be found at: */opt/net-controller/net-controllerd/.serverconfig*

    
    
<br><br><br><br><hr>
## Screenshots:
<br>

![overview](https://raw.githubusercontent.com/jtaylorthegreat/RemoteCLI/master/Screenshots/screenshot1.jpg)

![agents list](https://raw.githubusercontent.com/jtaylorthegreat/RemoteCLI/master/Screenshots/screenshot2.jpg)

![agent information/actions](https://raw.githubusercontent.com/jtaylorthegreat/RemoteCLI/master/Screenshots/screenshot3.jpg)


