#!/bin/sh

source /etc/tizen-platform.conf

sqlite3 $TZ_USER_DB/.browser-history.db 'PRAGMA journal_mode=PERSIST;
	CREATE TABLE history(
		id INTEGER PRIMARY KEY AUTOINCREMENT
		, address
		, title
		, counter INTEGER
		, visitdate DATETIME
		, favicon BLOB
		, favicon_length INTEGER
		, favicon_w INTEGER
		, favicon_h INTEGER
		, snapshot BLOB
		, snapshot_stride INTEGER
		, snapshot_w INTEGER
		, snapshot_h INTEGER);'


# Change db file owner & permission
chown :$TZ_SYS_USER_GROUP $TZ_USER_DB/.browser-history.db
chown :$TZ_SYS_USER_GROUP $TZ_USER_DB/.browser-history.db-journal
chmod 666 $TZ_USER_DB/.browser-history.db
chmod 666 $TZ_USER_DB/.browser-history.db-journal

