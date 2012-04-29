/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */

#include <string.h>
#include <dlog.h>
#include <db-util.h>
#include <favorites.h>
#include <favorites_private.h>

__thread sqlite3 *gl_internet_bookmark_db = 0;

/* Private Functions */
void _favorites_close_bookmark_db(void)
{
	if (gl_internet_bookmark_db) {
		/* ASSERT(currentThread() == m_openingThread); */
		db_util_close(gl_internet_bookmark_db);
		gl_internet_bookmark_db = 0;
	}
}

void _favorites_finalize_bookmark_db(sqlite3_stmt *stmt)
{
	if (sqlite3_finalize(stmt) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_finalize is failed");
	}
	_favorites_close_bookmark_db();
}
const char *_favorites_get_bookmark_db_name(void)
{
	return "/opt/dbspace/.internet_bookmark.db";
}
int _favorites_open_bookmark_db(void)
{
	_favorites_close_bookmark_db();
	if (db_util_open
	    (_favorites_get_bookmark_db_name(), &gl_internet_bookmark_db,
	     DB_UTIL_REGISTER_HOOK_METHOD) != SQLITE_OK) {
		db_util_close(gl_internet_bookmark_db);
		gl_internet_bookmark_db = 0;
		return -1;
	}
	return gl_internet_bookmark_db ? 0 : -1;
}

void _favorites_free_bookmark_list(bookmark_list_h m_list)
{
	FAVORITES_LOGE(" ");

	int i = 0;
	if (m_list == NULL)
		return;

	if (m_list->item != NULL) {
		for (i = 0; i < m_list->count; i++) {
			if (m_list->item[i].address != NULL)
				free(m_list->item[i].address);
			if (m_list->item[i].title != NULL)
				free(m_list->item[i].title);
			if (m_list->item[i].creationdate != NULL)
				free(m_list->item[i].creationdate);
			if (m_list->item[i].updatedate != NULL)
				free(m_list->item[i].updatedate);
		}
		free(m_list->item);
	}
	free(m_list);
	m_list = NULL;
}

int _favorites_free_bookmark_entry(favorites_bookmark_entry_s *entry)
{
	FAVORITES_NULL_ARG_CHECK(entry);

	if (entry->address != NULL)
		free(entry->address);
	if (entry->title != NULL)
		free(entry->title);
	if (entry->creation_date != NULL)
		free(entry->creation_date);
	if (entry->update_date != NULL)
		free(entry->update_date);

	return FAVORITES_ERROR_NONE;
}

/* search last of sequence(order's index) */
int _favorites_get_bookmark_lastindex(int locationId)
{
	int nError;
	sqlite3_stmt *stmt;

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return -1;
	}

	nError =
	    sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "select sequence from bookmarks where parent=? order by sequence desc",
			       -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}
	if (sqlite3_bind_int(stmt, 1, locationId) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_int is failed");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	if ((nError = sqlite3_step(stmt)) == SQLITE_ROW) {
		int index = sqlite3_column_int(stmt, 0);
		_favorites_finalize_bookmark_db(stmt);
		return index;
	}
	FAVORITES_LOGE("Not found items in This Folder");
	_favorites_close_bookmark_db();
	return 0;
}

int _favorites_bookmark_get_folderID(const char *foldername)
{
	FAVORITES_LOGE("foldername: %s", foldername);
	int nError;
	sqlite3_stmt *stmt;
	char	query[1024];

	memset(&query, 0x00, sizeof(char)*1024);

	/* If there is no identical folder name, create new folder */
	if (_favorites_bookmark_is_folder_exist(foldername)==0){
		if(_favorites_bookmark_create_folder(foldername)!=1){
			FAVORITES_LOGE("folder creating is failed.");
			return -1;
		}
	}

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return -1;
	}

	sprintf(query, "select id from bookmarks where type=1 AND title='%s'"
			, foldername);

	/* check foldername in the bookmark table */
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,query
				,-1, &stmt, NULL);

	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		int folderId = sqlite3_column_int(stmt, 0);
		_favorites_finalize_bookmark_db(stmt);
		return folderId;
	}
	_favorites_close_bookmark_db();
	return 1;
}

int _favorites_bookmark_is_folder_exist(const char *foldername)
{
	FAVORITES_LOGE("\n");
	int nError;
	sqlite3_stmt *stmt;
	char	query[1024];

	memset(&query, 0x00, sizeof(char)*1024);

	if (!foldername || (strlen(foldername) <= 0)) {
		FAVORITES_LOGE("foldername is empty\n");
		return -1;
	}
	FAVORITES_LOGE("foldername: %s", foldername);

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return -1;
	}

	sprintf(query, "select id from bookmarks where type=1 AND title='%s'"
			, foldername);

	/* check foldername in the bookmark table */
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,query
			       ,-1, &stmt, NULL);

	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		/* The given foldername is exist on the bookmark table */
		_favorites_finalize_bookmark_db(stmt);
		return 1;
	}
	_favorites_close_bookmark_db();
	return 0;
}

int _favorites_bookmark_create_folder(const char *foldername)
{
	FAVORITES_LOGE("\n");
	int nError;
	sqlite3_stmt *stmt;
	char	query[1024];
	int lastIndex = 0;

	memset(&query, 0x00, sizeof(char)*1024);

	if (!foldername || (strlen(foldername) <= 0)) {
		FAVORITES_LOGE("foldername is empty\n");
		return -1;
	}

	if ((lastIndex = _favorites_get_bookmark_lastindex(1)) < 0) {
		FAVORITES_LOGE("Database::getLastIndex() is failed.\n");
		return -1;
	}

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return -1;
	}

	sprintf(query, "insert into bookmarks \
		(type, parent, title, creationdate, sequence, updatedate, editable)\
		values (1, 1, '%s', DATETIME('now'), %d, DATETIME('now'), 1)"
		, foldername, lastIndex);

	FAVORITES_LOGE("query:%s\n", query);

	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			        query, -1, &stmt, NULL);

	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}
	
	nError = sqlite3_step(stmt);
	if (nError == SQLITE_OK || nError == SQLITE_DONE) {
		_favorites_finalize_bookmark_db(stmt);
		return 1;
	}
	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_close_bookmark_db();
	return 0;
}

int _favorites_bookmark_is_bookmark_exist
	(const char *url, const char *title, const int folderId)
{
	FAVORITES_LOGE("folderId: %d", folderId);
	int nError;
	sqlite3_stmt *stmt;
	char	query[1024];

	memset(&query, 0x00, sizeof(char)*1024);
		
	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return -1;
	}

	sprintf(query, "select id from bookmarks where \
			type=0 AND address='%s' AND title='%s' AND parent=%d"
			, url, title, folderId);
	FAVORITES_LOGE("query: %s", query);

	/* check bookmark in the bookmark table */
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,query
			       ,-1, &stmt, NULL);

	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		/* There is same bookmark exist. */
		_favorites_finalize_bookmark_db(stmt);
		return 1;
	}

	FAVORITES_LOGE("there is no identical bookmark\n");
	_favorites_close_bookmark_db();
	/* there is no identical bookmark*/
	return 0;
}

int _favorites_get_bookmark_count_at_folder(int folderId)
{
	int nError;
	sqlite3_stmt *stmt;
	FAVORITES_LOGE("");
	
	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return -1;
	}

	/*bookmark */
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "select count(*) from bookmarks where parent=? and type=0",
			       -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	if (sqlite3_bind_int(stmt, 1, folderId) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_int is failed");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		int count = sqlite3_column_int(stmt, 0);
		_favorites_finalize_bookmark_db(stmt);
		FAVORITES_LOGE("count: %d", count);
		return count;
	}
	_favorites_close_bookmark_db();
	FAVORITES_LOGE("End");
	return 0;
}

int _favorites_bookmark_get_folder_count(void)
{
	int nError;
	sqlite3_stmt *stmt;
	FAVORITES_LOGE("");
	
	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return -1;
	}

	/* folder + bookmark */
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "select count(*) from bookmarks where type=1 and parent != 0",
			       -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		int count = sqlite3_column_int(stmt, 0);
		_favorites_finalize_bookmark_db(stmt);
		return count;
	}
	_favorites_close_bookmark_db();
	return 0;
}

bookmark_list_h _favorites_get_bookmark_list_at_folder(int folderId)
{
	bookmark_list_h m_list = NULL;
	int nError;
	sqlite3_stmt *stmt;
	char	query[1024];

	FAVORITES_LOGE("folderId: %d", folderId);
	if(folderId<=0){
		FAVORITES_LOGE("folderId is wrong");
		return NULL;
	}

	memset(&query, 0x00, sizeof(char)*1024);

	/* check the total count of items */
	int item_count = 0;
	item_count = _favorites_get_bookmark_count_at_folder(folderId);

	if (item_count <= 0)
		return NULL;

	/* Get bookmarks list only under given folder */
	sprintf(query, "select id, type, parent, address, title, editable,\
			       creationdate, updatedate, sequence \
			       from bookmarks where type=0 and parent =%d order by sequence"
			, folderId);
	FAVORITES_LOGE("query: %s", query);

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return NULL;
	}
	
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			       query, -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE ("sqlite3_prepare_v2 is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return NULL;
	}

	/*  allocation .... Array for Items */
	m_list = (bookmark_list_h) calloc(1, sizeof(bookmark_list_s));
	m_list->item =
	    (bookmark_entry_internal_h) calloc(item_count, sizeof(bookmark_entry_internal_s));
	m_list->count = item_count;
	int i = 0;
	while ((nError = sqlite3_step(stmt)) == SQLITE_ROW 
		&& (i < item_count)) {
		m_list->item[i].id = sqlite3_column_int(stmt, 0);
		m_list->item[i].is_folder = sqlite3_column_int(stmt, 1);
		m_list->item[i].folder_id = sqlite3_column_int(stmt, 2);

		if (!m_list->item[i].is_folder) {
			const char *url =
			    (const char *)(sqlite3_column_text(stmt, 3));
			m_list->item[i].address = NULL;
			if (url) {
				int length = strlen(url);
				if (length > 0) {
					m_list->item[i].address =
					    (char *)calloc(length + 1,
							   sizeof(char));
					memcpy(m_list->item[i].address, url,
					       length);
				}
			}
		}

		const char *title =
		    (const char *)(sqlite3_column_text(stmt, 4));
		m_list->item[i].title = NULL;
		if (title) {
			int length = strlen(title);
			if (length > 0) {
				m_list->item[i].title =
				    (char *)calloc(length + 1, sizeof(char));
				memcpy(m_list->item[i].title, title, length);
			}
			FAVORITES_LOGE("Bookmark Title:%s\n", m_list->item[i].title);
		}
		m_list->item[i].editable = sqlite3_column_int(stmt, 5);

		const char *creationdate =
		    (const char *)(sqlite3_column_text(stmt, 6));
		m_list->item[i].creationdate = NULL;
		if (creationdate) {
			int length = strlen(creationdate);
			if (length > 0) {
				m_list->item[i].creationdate =
				    (char *)calloc(length + 1, sizeof(char));
				memcpy(m_list->item[i].creationdate,
				       creationdate, length);
			}
		}
		const char *updatedate =
		    (const char *)(sqlite3_column_text(stmt, 7));
		m_list->item[i].updatedate = NULL;
		if (updatedate) {
			int length = strlen(updatedate);
			if (length > 0) {
				m_list->item[i].updatedate =
				    (char *)calloc(length + 1, sizeof(char));
				memcpy(m_list->item[i].updatedate, updatedate,
				       length);
			}
		}

		m_list->item[i].orderIndex = sqlite3_column_int(stmt, 8);
		i++;
	}
	m_list->count = i;

	if (i <= 0) {
		FAVORITES_LOGE("sqlite3_step is failed");
		_favorites_close_bookmark_db();
		_favorites_free_bookmark_list(m_list);
		return NULL;
	}
	_favorites_finalize_bookmark_db(stmt);
	return m_list;
}

bookmark_list_h _favorites_bookmark_get_folder_list(void)
{
	bookmark_list_h m_list = NULL;
	int nError;
	sqlite3_stmt *stmt;
	char	query[1024];

	FAVORITES_LOGE("");

	memset(&query, 0x00, sizeof(char)*1024);

	/* check the total count of items */
	int item_count = 0;
	item_count = _favorites_bookmark_get_folder_count();

	if (item_count <= 0)
		return NULL;

	/* Get bookmarks list only under given folder */
	sprintf(query, "select id, type, parent, address, title, editable,\
			       creationdate, updatedate, sequence \
			       from bookmarks where type=1 and parent != 0 order by sequence");
	FAVORITES_LOGE("query: %s", query);

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return NULL;
	}
	
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			       query, -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE ("sqlite3_prepare_v2 is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return NULL;
	}

	/*  allocation .... Array for Items */
	m_list = (bookmark_list_h) calloc(1, sizeof(bookmark_list_s));
	m_list->item =
	    (bookmark_entry_internal_h) calloc(item_count, sizeof(bookmark_entry_internal_s));
	m_list->count = item_count;
	int i = 0;
	while ((nError = sqlite3_step(stmt)) == SQLITE_ROW 
		&& (i < item_count)) {
		m_list->item[i].id = sqlite3_column_int(stmt, 0);
		m_list->item[i].is_folder = sqlite3_column_int(stmt, 1);
		m_list->item[i].folder_id = sqlite3_column_int(stmt, 2);

		if (!m_list->item[i].is_folder) {
			const char *url =
			    (const char *)(sqlite3_column_text(stmt, 3));
			m_list->item[i].address = NULL;
			if (url) {
				int length = strlen(url);
				if (length > 0) {
					m_list->item[i].address =
					    (char *)calloc(length + 1,
							   sizeof(char));
					memcpy(m_list->item[i].address, url,
					       length);
				}
			}
		}

		const char *title =
		    (const char *)(sqlite3_column_text(stmt, 4));
		m_list->item[i].title = NULL;
		if (title) {
			int length = strlen(title);
			if (length > 0) {
				m_list->item[i].title =
				    (char *)calloc(length + 1, sizeof(char));
				memcpy(m_list->item[i].title, title, length);
			}
			FAVORITES_LOGE("Bookmark Title:%s\n", m_list->item[i].title);
		}
		m_list->item[i].editable = sqlite3_column_int(stmt, 5);

		const char *creationdate =
		    (const char *)(sqlite3_column_text(stmt, 6));
		m_list->item[i].creationdate = NULL;
		if (creationdate) {
			int length = strlen(creationdate);
			if (length > 0) {
				m_list->item[i].creationdate =
				    (char *)calloc(length + 1, sizeof(char));
				memcpy(m_list->item[i].creationdate,
				       creationdate, length);
			}
		}
		const char *updatedate =
		    (const char *)(sqlite3_column_text(stmt, 7));
		m_list->item[i].updatedate = NULL;
		if (updatedate) {
			int length = strlen(updatedate);
			if (length > 0) {
				m_list->item[i].updatedate =
				    (char *)calloc(length + 1, sizeof(char));
				memcpy(m_list->item[i].updatedate, updatedate,
				       length);
			}
		}

		m_list->item[i].orderIndex = sqlite3_column_int(stmt, 8);
		i++;
	}
	m_list->count = i;

	if (i <= 0) {
		FAVORITES_LOGE("sqlite3_step is failed");
		_favorites_close_bookmark_db();
		_favorites_free_bookmark_list(m_list);
		return NULL;
	}
	_favorites_finalize_bookmark_db(stmt);
	return m_list;
}

int _favorites_get_unixtime_from_datetime(char *datetime)
{
	int nError;
	sqlite3_stmt *stmt;

	if(datetime == NULL ) {
		FAVORITES_LOGE("datetime is NULL\n");
		return -1;
	}

	FAVORITES_LOGE("datetime: %s\n", datetime);

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return -1;
	}

	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "SELECT strftime('%s', ?)",
			       -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	if (sqlite3_bind_text(stmt, 1, datetime, -1, NULL) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_text is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		int unixtime = sqlite3_column_int(stmt, 0);
		_favorites_finalize_bookmark_db(stmt);
		return unixtime;
	}
	_favorites_close_bookmark_db();
	return 1;
}

/* search last of sequence(order's index) */
int _favorites_bookmark_get_bookmark_id(const char *url, const char *title, const int folder_id)
{
	int nError;
	sqlite3_stmt *stmt;
	char	query[1024];

	/* Get the id of bookmark */
	sprintf(query, "select id from bookmarks where \
			type=0 AND address='%s' AND title='%s' AND parent=%d"
			, url, title, folder_id);
	FAVORITES_LOGE("query: %s", query);

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return -1;
	}

	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
				query, -1, &stmt, NULL);

	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	if ((nError = sqlite3_step(stmt)) == SQLITE_ROW) {
		int bookmark_id = sqlite3_column_int(stmt, 0);
		_favorites_finalize_bookmark_db(stmt);
		return bookmark_id;
	}
	FAVORITES_LOGE("No match with given url");
	_favorites_close_bookmark_db();
	return 0;
}

/*************************************************************
 *	APIs for Internet favorites
 *************************************************************/
int favorites_bookmark_add_bookmark(const char *url, const char *title, const char *foldername, int *bookmark_id)
{
	FAVORITES_LOGE("");
	int nError;
	sqlite3_stmt *stmt;
	int folderId = 1;
	char	query[1024];
	int lastIndex = 0;

	memset(&query, 0x00, sizeof(char)*1024);
	if (!url || (strlen(url) <= 0)) {
		FAVORITES_LOGE("url is empty\n");
		return FAVORITES_ERROR_INVALID_PARAMETER;
	}

	if (!title || (strlen(title) <= 0)) {
		FAVORITES_LOGE("title is empty\n");
		return FAVORITES_ERROR_INVALID_PARAMETER;
	}

	/* check the foldername is exist and get a folderid */
	if (!foldername || (strlen(foldername) <= 0)) {
		FAVORITES_LOGE("foldername is empty. id is now root.\n");
		folderId = 1;
	}else if (!strcmp("Bookmarks", foldername)){
		/*root folder name is "Bookmarks".*/
		folderId = 1;
	} else {
		folderId = _favorites_bookmark_get_folderID(foldername);
		if(folderId<0){
			return FAVORITES_ERROR_DB_FAILED;
		}
	}

	/* Check the bookmarks is already exist*/
	if(_favorites_bookmark_is_bookmark_exist(url, title, folderId)!=0){
		FAVORITES_LOGE("The bookmark is already exist.\n");
		return FAVORITES_ERROR_ITEM_ALREADY_EXIST;
	}

	/* get a last index for order of bookmark items */
	if ((lastIndex = _favorites_get_bookmark_lastindex(folderId)) < 0) {
		FAVORITES_LOGE("Database::getLastIndex() is failed.\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	/* Creating SQL query sentence. */
	sprintf(query, "insert into bookmarks\
			(type, parent, address, title, creationdate, editable, sequence, accesscount)\
			values(0, %d, '%s', '%s',  DATETIME('now'), 1, %d, 0);"
			, folderId, url, title, lastIndex);

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
				query, -1, &stmt, NULL);

	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	
	nError = sqlite3_step(stmt);
	if (nError == SQLITE_OK || nError == SQLITE_DONE) {
		_favorites_finalize_bookmark_db(stmt);
		if (bookmark_id != NULL) {
			*bookmark_id = _favorites_bookmark_get_bookmark_id(url, title, folderId);
		}
		return FAVORITES_ERROR_NONE;
	}
	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_close_bookmark_db();

	return FAVORITES_ERROR_DB_FAILED;
}

int favorites_bookmark_get_count(int *count)
{
	int nError;
	sqlite3_stmt *stmt;

	FAVORITES_NULL_ARG_CHECK(count);
	
	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	/* folder + bookmark */
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "select count(*) from bookmarks where parent != 0",
			       -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		*count = sqlite3_column_int(stmt, 0);
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_NONE;
	}
	_favorites_close_bookmark_db();
	return FAVORITES_ERROR_DB_FAILED;
}

int favorites_bookmark_foreach(favorites_bookmark_foreach_cb callback,void *user_data)
{
	FAVORITES_NULL_ARG_CHECK(callback);
	int nError;
	int func_ret = 0;
	sqlite3_stmt *stmt;

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "select id, type, parent, address, title, editable,\
			       creationdate, updatedate, sequence \
			       from bookmarks where parent != 0 order by sequence",
			       -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE ("sqlite3_prepare_v2 is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}

	while ((nError = sqlite3_step(stmt)) == SQLITE_ROW) {
		favorites_bookmark_entry_s result;
		memset(&result, 0x00, sizeof(favorites_bookmark_entry_s));
		result.id = sqlite3_column_int(stmt, 0);
		result.is_folder = sqlite3_column_int(stmt, 1);
		result.folder_id = sqlite3_column_int(stmt, 2);

		result.address = NULL;
		if (!result.is_folder) {
			const char *url = (const char *)(sqlite3_column_text(stmt, 3));
			if (url) {
				int length = strlen(url);
				if (length > 0) {
					result.address = (char *)calloc(length + 1, sizeof(char));
					memcpy(result.address, url, length);
					FAVORITES_LOGE ("url:%s\n", url);
				}
			}
		}

		const char *title = (const char *)(sqlite3_column_text(stmt, 4));
		result.title = NULL;
		if (title) {
			int length = strlen(title);
			if (length > 0) {
				result.title = (char *)calloc(length + 1, sizeof(char));
				memcpy(result.title, title, length);
			}
		}
		result.editable = sqlite3_column_int(stmt, 5);

		const char *creation_date = (const char *)(sqlite3_column_text(stmt, 6));
		result.creation_date = NULL;
		if (creation_date) {
			int length = strlen(creation_date);
			if (length > 0) {
				result.creation_date = (char *)calloc(length + 1, sizeof(char));
				memcpy(result.creation_date, creation_date, length);
			}
		}
		const char *update_date = (const char *)(sqlite3_column_text(stmt, 7));
		result.update_date = NULL;
		if (update_date) {
			int length = strlen(update_date);
			if (length > 0) {
				result.update_date = (char *)calloc(length + 1, sizeof(char));
				memcpy(result.update_date, update_date, length);
			}
		}

		result.order_index = sqlite3_column_int(stmt, 8);

		func_ret = callback(&result, user_data);
		_favorites_free_bookmark_entry(&result);
		if(func_ret == 0) 
			break;
	}

	FAVORITES_LOGE ("There are no more bookmarks.\n");
	_favorites_finalize_bookmark_db(stmt);
	return FAVORITES_ERROR_NONE;
}

int favorites_bookmark_export_list(const char * file_path)
{
	FAVORITES_NULL_ARG_CHECK(file_path);
	FILE *fp = NULL;
	bookmark_list_h folders_list = NULL;
	bookmark_list_h bookmarks_list = NULL;

	/* Get list of all bookmarks */
	folders_list = _favorites_bookmark_get_folder_list();
	if(folders_list == NULL ) {
		FAVORITES_LOGE("There is no folders even root folder");
		return FAVORITES_ERROR_DB_FAILED;
	}

	fp = fopen( file_path, "w");
	if(fp == NULL) {
		FAVORITES_LOGE("file opening is failed.");
		return FAVORITES_ERROR_INVALID_PARAMETER;
	}
	fputs("<!DOCTYPE NETSCAPE-Bookmark-file-1>\n", fp);
	fputs("<!-- This is an automatically generated file.\n", fp);
	fputs("It will be read and overwritten.\n", fp);
	fputs("Do Not Edit! -->\n", fp);
	fputs("<META HTTP-EQUIV=\"Content-Type\" ", fp);
	fputs("CONTENT=\"text/html; charset=UTF-8\">\n", fp);
	fputs("<TITLE>Bookmarks</TITLE>\n", fp);
	fputs("<H1>Bookmarks</H1>\n", fp);
	fputs("<DL><p>\n", fp);
	/*Set subfolders and its bookmark items */
	int i = 0;
	int folder_adddate_unixtime = 0;
	int k=0;
	int bookmark_adddate_unixtime = 0;
	int bookmark_updatedate_unixtime = 0;
	for(i=0; i < (folders_list->count); i++) {
		folder_adddate_unixtime =
			_favorites_get_unixtime_from_datetime(
				folders_list->item[i].creationdate);
		FAVORITES_LOGE("TITLE: %s", folders_list->item[i].title);

		fprintf(fp, "\t<DT><H3 FOLDED ADD_DATE=\"%d\">%s</H3>\n", 
				folder_adddate_unixtime, folders_list->item[i].title);
		fputs("\t<DL><p>\n", fp);
		/* Get bookmarks under this folder and put the list into the file*/
		_favorites_free_bookmark_list(bookmarks_list);
		bookmarks_list = NULL;
		bookmarks_list = _favorites_get_bookmark_list_at_folder(
						folders_list->item[i].id);
		if(bookmarks_list!= NULL){
			for(k=0;k<(bookmarks_list->count); k++){
				bookmark_adddate_unixtime =
					_favorites_get_unixtime_from_datetime(
					bookmarks_list->item[k].creationdate);

				if(bookmark_adddate_unixtime<0)
					bookmark_adddate_unixtime=0;

				bookmark_updatedate_unixtime =
					_favorites_get_unixtime_from_datetime(
					bookmarks_list->item[k].updatedate);

				if(bookmark_updatedate_unixtime<0)
					bookmark_updatedate_unixtime=0;

				fprintf(fp,"\t\t<DT><A HREF=\"%s\" ", 
						bookmarks_list->item[k].address);
				fprintf(fp,"ADD_DATE=\"%d\" ",
						bookmark_adddate_unixtime);
				fprintf(fp,"LAST_VISIT=\"%d\" ",
						bookmark_updatedate_unixtime);
				fprintf(fp,"LAST_MODIFIED=\"%d\">",
						bookmark_updatedate_unixtime);
				fprintf(fp,"%s</A>\n", bookmarks_list->item[k].title );
			}
		}
		fputs("\t</DL><p>\n", fp);
	}

	/*Set root folder's bookmark items */
	_favorites_free_bookmark_list(bookmarks_list);
	bookmarks_list = NULL;
	bookmarks_list = _favorites_get_bookmark_list_at_folder(1);
	if(bookmarks_list!= NULL){
		for(k=0;k<(bookmarks_list->count); k++){
			bookmark_adddate_unixtime =
				_favorites_get_unixtime_from_datetime(
				bookmarks_list->item[k].creationdate);

			if(bookmark_adddate_unixtime<0)
				bookmark_adddate_unixtime=0;

			bookmark_updatedate_unixtime =
				_favorites_get_unixtime_from_datetime(
				bookmarks_list->item[k].updatedate);

			if(bookmark_updatedate_unixtime<0)
				bookmark_updatedate_unixtime=0;

			fprintf(fp,"\t<DT><A HREF=\"%s\" ", 
					bookmarks_list->item[k].address);
			fprintf(fp,"ADD_DATE=\"%d\" ",
					bookmark_adddate_unixtime);
			fprintf(fp,"LAST_VISIT=\"%d\" ",
					bookmark_updatedate_unixtime);
			fprintf(fp,"LAST_MODIFIED=\"%d\">",
					bookmark_updatedate_unixtime);
			fprintf(fp,"%s</A>\n", bookmarks_list->item[k].title );
		}
	}
	fputs("</DL><p>\n", fp);
	fclose(fp);

	return FAVORITES_ERROR_NONE;
}

int favorites_bookmark_get_favicon(int id, Evas *evas, Evas_Object **icon)
{
	FAVORITES_INVALID_ARG_CHECK(id<0);
	FAVORITES_NULL_ARG_CHECK(evas);
	FAVORITES_NULL_ARG_CHECK(icon);

	sqlite3_stmt *stmt;
	char	query[1024];
	void *favicon_data_temp=NULL;
	favicon_entry_h favicon;
	int nError;

	memset(&query, 0x00, sizeof(char)*1024);
	sprintf(query, "select favicon, favicon_length, favicon_w, favicon_h from bookmarks\
			where id=%d"
			, id);
	FAVORITES_LOGE("query: %s", query);

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			query, -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		favicon = (favicon_entry_h) calloc(1, sizeof(favicon_entry_s));
		/* loading favicon from bookmark db */
		favicon_data_temp = (void *)sqlite3_column_blob(stmt,0);
		favicon->length = sqlite3_column_int(stmt,1);
		favicon->w = sqlite3_column_int(stmt,2);
		favicon->h = sqlite3_column_int(stmt,3);

		if (favicon->length > 0){
			favicon->data = calloc(1, favicon->length);
			memcpy(favicon->data, favicon_data_temp, favicon->length);
			/* transforming to evas object */
			*icon = evas_object_image_filled_add(evas);
			evas_object_image_colorspace_set(*icon,
							EVAS_COLORSPACE_ARGB8888);
			evas_object_image_size_set(*icon, favicon->w, favicon->h);
			evas_object_image_fill_set(*icon, 0, 0, favicon->w,
									favicon->h);
			evas_object_image_filled_set(*icon, EINA_TRUE);
			evas_object_image_alpha_set(*icon,EINA_TRUE);
			evas_object_image_data_set(*icon, favicon->data);
		}
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_NONE;
	}

	_favorites_close_bookmark_db();
	return FAVORITES_ERROR_NONE;
}

int favorites_bookmark_delete_bookmark(int id)
{
	FAVORITES_INVALID_ARG_CHECK(id<0);
	int nError;
	sqlite3_stmt *stmt;

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			        "delete from bookmarks where id=? and parent != 0", -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	// bind
	if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_int is failed");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	nError = sqlite3_step(stmt);
	if (nError == SQLITE_OK || nError == SQLITE_DONE) {
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_NONE;
	}
	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_close_bookmark_db();
	return FAVORITES_ERROR_DB_FAILED;
}

int favorites_bookmark_delete_all_bookmarks(void)
{
	int nError;
	sqlite3_stmt *stmt;

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			        "delete from bookmarks where parent !=0", -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	nError = sqlite3_step(stmt);
	if (nError == SQLITE_OK || nError == SQLITE_DONE) {
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_NONE;
	}
	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_close_bookmark_db();
	return FAVORITES_ERROR_DB_FAILED;
}

