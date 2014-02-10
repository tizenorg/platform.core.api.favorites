#!/bin/sh

source /etc/tizen-platform.conf

sqlite3 $TZ_USER_DB/.internet_bookmark.db 'PRAGMA journal_mode=PERSIST;
	CREATE TABLE bookmarks(
		id INTEGER PRIMARY KEY AUTOINCREMENT
		,type INTEGER
		,parent INTEGER
		,address
		,title
		,creationdate
		,sequence INTEGER
		,updatedate
		,visitdate
		,editable INTEGER
		,accesscount INTEGER
		,favicon BLOB
		,favicon_length INTEGER
		,favicon_w INTEGER
		,favicon_h INTEGER
		,created_date
		,account_name
		,account_type
		,thumbnail BLOB
		,thumbnail_length INTEGER
		,thumbnail_w INTEGER
		,thumbnail_h INTEGER
		,version INTEGER
		,sync
		,tag1
		,tag2
		,tag3
		,tag4
	);
	create index idx_bookmarks_on_parent_type on bookmarks(parent, type);

	CREATE TABLE tags(
		tag
	);'

chown :$TZ_SYS_USER_GROUP $TZ_USER_DB/.internet_bookmark.db
chown :$TZ_SYS_USER_GROUP $TZ_USER_DB/.internet_bookmark.db-journal
chmod 666 $TZ_USER_DB/.internet_bookmark.db
chmod 666 $TZ_USER_DB/.internet_bookmark.db-journal
