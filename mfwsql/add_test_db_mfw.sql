USE db_mfw;

/* mfw框架 */
INSERT INTO t_server VALUES("mfw", "mfwregistry", "", "192.168.1.254", 0);
INSERT INTO t_service VALUES("mfw", "mfwregistry", "", "192.168.1.254", "QueryObj", "tcp -h 192.168.1.254 -p 2000");

INSERT INTO t_server VALUES("mfw", "mfwlog", "", "192.168.1.254", 0);
INSERT INTO t_service VALUES("mfw", "mfwlog", "", "192.168.1.254", "LogObj", "tcp -h 192.168.1.254 -p 2001");

/* 全局 */
INSERT INTO t_server VALUES("MTTD", "LoginServer", "", "192.168.1.254", 0);
INSERT INTO t_service VALUES("MTTD", "LoginServer", "", "192.168.1.254", "HandleLogin", "tcp -h 192.168.1.254 -p 3000");

INSERT INTO t_server VALUES("MTTD", "DirServer", "", "192.168.1.254", 0);
INSERT INTO t_service VALUES("MTTD", "DirServer", "", "192.168.1.254", "DirServiceObj", "tcp -h 192.168.1.254 -p 3001");

INSERT INTO t_server VALUES("MTTD", "AccountServer", "", "192.168.1.254", 0);
INSERT INTO t_service VALUES("MTTD", "AccountServer", "", "192.168.1.254", "AccountServiceObj", "tcp -h 192.168.1.254 -p 3002");

INSERT INTO t_server VALUES("MTTD", "DirtyCheckServer", "", "192.168.1.254", 0);
INSERT INTO t_service VALUES("MTTD", "DirtyCheckServer", "", "192.168.1.254", "DirtyCheckServiceObj", "tcp -h 192.168.1.254 -p 3003");

/* 分区 */
INSERT INTO t_server VALUES("MTTD", "ConnServer", "mttd.zone.20", "192.168.1.254", 0);
INSERT INTO t_service VALUES("MTTD", "ConnServer", "mttd.zone.20", "192.168.1.254", "HandleConn", "tcp -h 192.168.1.254 -p 4000");
INSERT INTO t_service VALUES("MTTD", "ConnServer", "mttd.zone.20", "192.168.1.254", "ConnServiceObj", "tcp -h 192.168.1.254 -p 4001");

INSERT INTO t_server VALUES("MTTD", "GameServer", "mttd.zone.20", "192.168.1.254", 0);
INSERT INTO t_service VALUES("MTTD", "GameServer", "mttd.zone.20", "192.168.1.254", "GameServiceObj", "tcp -h 192.168.1.254 -p 4002");
