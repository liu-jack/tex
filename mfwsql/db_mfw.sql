CREATE DATABASE IF NOT EXISTS db_mfw;
USE db_mfw;

CREATE TABLE IF NOT EXISTS t_server
(
	app                     VARCHAR(64) NOT NULL,
	server                  VARCHAR(64) NOT NULL,
	division                VARCHAR(64) NOT NULL,
	node                    VARCHAR(64) NOT NULL,
	status                  INT UNSIGNED NOT NULL,
	PRIMARY KEY(app, server, division, node)
);

CREATE TABLE IF NOT EXISTS t_service
(
	app                     VARCHAR(64) NOT NULL,
	server                  VARCHAR(64) NOT NULL,
	division                VARCHAR(64) NOT NULL,
	node                    VARCHAR(64) NOT NULL,
	service                 VARCHAR(128) NOT NULL,
	endpoint                VARCHAR(512) NOT NULL,
	PRIMARY KEY(app, server, division, node, service)
);
