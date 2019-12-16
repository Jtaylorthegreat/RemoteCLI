# RemoteCLI

 Remote-CLI provides a web management portal to execute command line commands and remotely transfer scripts to your cloud servers. 







# Installation Guide:

## **Net-Controller**

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

## Agent Installation

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
