/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd All Rights Reserved
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
#if defined(BROWSER_BOOKMARK_SYNC)
#include "bookmark-adaptor.h"
#include "time.h"
#endif

sqlite3 *gl_internet_bookmark_db = 0;

/* Private Functions */
void _favorites_close_bookmark_db(void)
{
	if (gl_internet_bookmark_db) {
		if (db_util_close(gl_internet_bookmark_db) != DB_UTIL_OK) {
			FAVORITES_LOGE("db_util_close is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		}
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
	return "/opt/usr/dbspace/.internet_bookmark.db";
}
int _favorites_open_bookmark_db(void)
{
	_favorites_close_bookmark_db();
	if (db_util_open
	    (_favorites_get_bookmark_db_name(), &gl_internet_bookmark_db,
	     DB_UTIL_REGISTER_HOOK_METHOD) != SQLITE_OK) {
		if (db_util_close(gl_internet_bookmark_db) != DB_UTIL_OK) {
			FAVORITES_LOGE("db_util_close is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		}
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
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
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
	} else if (nError == SQLITE_DONE) {
		FAVORITES_LOGE("Not found items in This Folder");
		_favorites_finalize_bookmark_db(stmt);
		return 0;
	}
	_favorites_close_bookmark_db();
	return -1;
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
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
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
	} else if (nError == SQLITE_DONE) {
		_favorites_finalize_bookmark_db(stmt);
		return 0;
	}
	_favorites_close_bookmark_db();
	return -1;
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
			       "select count(*) from bookmarks where type=1",
			       -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		int count = sqlite3_column_int(stmt, 0);
		_favorites_finalize_bookmark_db(stmt);
		return count;
	} else if (nError == SQLITE_DONE) {
		_favorites_finalize_bookmark_db(stmt);
		return 0;
	}
	_favorites_close_bookmark_db();
	return -1;
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
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
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
			       from bookmarks where type=1 order by sequence");
	FAVORITES_LOGE("query: %s", query);

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return NULL;
	}
	
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			       query, -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
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

	if (nError == SQLITE_DONE) {
		if (m_list->count > 0) {
			_favorites_finalize_bookmark_db(stmt);
			return m_list;
		} else if (m_list->count == 0) {
			FAVORITES_LOGE("There is no folders");
			_favorites_finalize_bookmark_db(stmt);
			_favorites_free_bookmark_list(m_list);
			return NULL;
		}
	}
	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_finalize_bookmark_db(stmt);
	_favorites_free_bookmark_list(m_list);
	return NULL;
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
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
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
	} else if (nError == SQLITE_DONE) {
		_favorites_finalize_bookmark_db(stmt);
		return 0;
	}
	_favorites_close_bookmark_db();
	return -1;
}

int _add_bookmark(const char *title, const char *address, int parent_id, int *saved_bookmark_id)
{
	FAVORITES_LOGD("");
	if (title == NULL || strlen(title) <= 0) {
		FAVORITES_LOGD("Invalid param: Title is empty");
		return FAVORITES_ERROR_INVALID_PARAMETER;
	}
	if (address == NULL || strlen(address) <= 0) {
		FAVORITES_LOGD("Invalid param: Address is empty");
		return FAVORITES_ERROR_INVALID_PARAMETER;
	}
	if (memcmp(address, "file://", 7) == 0) {
		FAVORITES_LOGD("Not Allow file://");
		return FAVORITES_ERROR_INVALID_PARAMETER;
	}
#if defined(BROWSER_BOOKMARK_SYNC)
	if (parent_id != _get_root_folder_id()) {
		/* check whether folder is exist or not. */
		if (_get_exists_id(parent_id) <= 0) {
			FAVORITES_LOGE("Not found parent folder (%d)\n", parent_id);
			return FAVORITES_ERROR_NO_SUCH_FILE;
		}
	}
	FAVORITES_LOGD("[%s][%s][%d]", title, address, parent_id);
	int id = -1;
	int *ids = NULL;
	int ids_count = -1;
	bp_bookmark_adaptor_get_duplicated_url_ids_p(&ids, &ids_count, 1,
		-1, -1, 0, 0, address, 0);
	if (ids_count > 0)
		id = ids[0];
	free(ids);

	if (ids_count > 0) {
		FAVORITES_LOGE("same URI is already exist");
		return FAVORITES_ERROR_ITEM_ALREADY_EXIST;
	}

	bp_bookmark_info_fmt info;
	memset(&info, 0x00, sizeof(bp_bookmark_info_fmt));
	info.type = 0;
	info.parent = parent_id;
	info.sequence = -1;
	info.access_count = -1;
	info.editable = 1;
	time_t seconds = 0;
	info.date_created = time(&seconds);
	info.date_modified = time(&seconds);
	if (address != NULL && strlen(address) > 0)
		info.url = strdup(address);
	if (title != NULL && strlen(title) > 0)
		info.title = strdup(title);

	int ret = bp_bookmark_adaptor_easy_create(&id, &info);
	if (ret == 0) {
		ret = bp_bookmark_adaptor_set_sequence(id, -1); // max sequence
		bp_bookmark_adaptor_easy_free(&info);
		if (ret == 0) {
			*saved_bookmark_id = id;
			FAVORITES_LOGD("bp_bookmark_adaptor_easy_create is success(id:%d)", *saved_bookmark_id);
			bp_bookmark_adaptor_publish_notification();
			return FAVORITES_ERROR_NONE;
		}
	}
	int errcode = bp_bookmark_adaptor_get_errorcode();
	bp_bookmark_adaptor_easy_free(&info);
	FAVORITES_LOGE("bp_bookmark_adaptor_easy_create is failed[%d]", errcode);
	return FAVORITES_ERROR_DB_FAILED;
#else
	int nError;
	sqlite3_stmt *stmt;
#if defined(ROOT_IS_ZERO)
	if (parent_id != _get_root_folder_id()) {
		/* check whether folder is exist or not. */
		if (_get_exists_id(parent_id) <= 0) {
			FAVORITES_LOGE("Not found parent folder (%d)\n", parent_id);
			return FAVORITES_ERROR_NO_SUCH_FILE;
		}
	}
#else
	if (parent_id > 0) {
		/* check whether folder is exist or not. */
		if (_get_exists_id(parent_id) <= 0) {
			FAVORITES_LOGE("Not found parent folder (%d)\n", parent_id);
			return FAVORITES_ERROR_NO_SUCH_FILE;
		}
	} else {
		parent_id = _get_root_folder_id();
	}
#endif
	FAVORITES_LOGD("[%s][%s][%d]", title, address, parent_id);

	int uid = _get_exists_bookmark(address);
	/* ignore this call.. already exist. */
	if (uid > 0) {
		FAVORITES_LOGE("Bookmark is already exist. cancel the add operation.");
		return FAVORITES_ERROR_ITEM_ALREADY_EXIST;
	}

	int lastIndex = _favorites_get_bookmark_lastindex(parent_id);
	if (lastIndex < 0) {
		FAVORITES_LOGE("Database::getLastIndex() is failed.\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError =
	    sqlite3_prepare_v2(gl_internet_bookmark_db,
					"INSERT INTO bookmarks (type, parent, address, title, creationdate,\
					updatedate, editable, sequence)\
					VALUES (0, ?, ?, ?, DATETIME('now'), DATETIME('now'), 1, ?)",
			       -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}

	/*parent */
	if (sqlite3_bind_int(stmt, 1, parent_id) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_int is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	/*address */
	if (sqlite3_bind_text(stmt, 2, address, -1, NULL) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_text is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	/*title */
	if (sqlite3_bind_text(stmt, 3, title, -1, NULL) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_text is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	/* order */
	if (lastIndex >= 0) {
		if (sqlite3_bind_int(stmt, 4, (lastIndex + 1)) != SQLITE_OK) {
			FAVORITES_LOGE("sqlite3_bind_int is failed.\n");
			_favorites_finalize_bookmark_db(stmt);
			return FAVORITES_ERROR_DB_FAILED;
		}
	}
	nError = sqlite3_step(stmt);
	if (nError == SQLITE_OK || nError == SQLITE_DONE) {
		_favorites_finalize_bookmark_db(stmt);
		int id = _get_bookmark_id(address);
		if (id > 0)
			*saved_bookmark_id = id;
		return FAVORITES_ERROR_NONE;
	}
	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_close_bookmark_db();
	return FAVORITES_ERROR_DB_FAILED;
#endif
}

int _add_folder(const char *title, int parent_id, int *saved_folder_id)
{
	FAVORITES_LOGD("");
	if (title == NULL || strlen(title) <= 0) {
		FAVORITES_LOGD("Invalid param: Title is empty");
		return FAVORITES_ERROR_INVALID_PARAMETER;
	}
#if defined(BROWSER_BOOKMARK_SYNC)
	if (parent_id != _get_root_folder_id()) {
		/* check whether folder is exist or not. */
		if (_get_exists_id(parent_id) <= 0) {
			FAVORITES_LOGE("Not found parent folder (%d)\n", parent_id);
			return FAVORITES_ERROR_NO_SUCH_FILE;
		}
	}
	int id = -1;
	int *ids = NULL;
	int ids_count = -1;
	bp_bookmark_adaptor_get_duplicated_title_ids_p(&ids, &ids_count, 1,
		-1, parent_id, 1, 0, title, 0);
	if (ids_count > 0)
		id = ids[0];
	free(ids);

	if (ids_count > 0) {
		FAVORITES_LOGE("same title with same parent is already exist");
		return FAVORITES_ERROR_ITEM_ALREADY_EXIST;
	}

	bp_bookmark_info_fmt info;
	memset(&info, 0x00, sizeof(bp_bookmark_info_fmt));
	info.type = 1;
	info.parent = parent_id;
	info.sequence = -1;
	info.access_count = -1;
	info.editable = 1;
	if (title != NULL && strlen(title) > 0)
		info.title = strdup(title);
	int ret = bp_bookmark_adaptor_easy_create(&id, &info);
	if (ret == 0) {
		ret = bp_bookmark_adaptor_set_sequence(id, -1); // max sequence
		bp_bookmark_adaptor_easy_free(&info);
		if (ret == 0) {
			*saved_folder_id = id;
			FAVORITES_LOGD("bmsvc_add_bookmark is success(id:%d)", *saved_folder_id);
			bp_bookmark_adaptor_publish_notification();
			return FAVORITES_ERROR_NONE;
		}
	}
	int errcode = bp_bookmark_adaptor_get_errorcode();
	bp_bookmark_adaptor_easy_free(&info);
	FAVORITES_LOGE("bp_bookmark_adaptor_easy_create is failed[%d]", errcode);
	return FAVORITES_ERROR_DB_FAILED;
#else
#if defined(ROOT_IS_ZERO)
	if (parent_id != _get_root_folder_id()) {
		/* check whether folder is exist or not. */
		if (_get_exists_id(parent_id) <= 0) {
			FAVORITES_LOGE("Not found parent folder (%d)\n", parent_id);
			return FAVORITES_ERROR_NO_SUCH_FILE;
		}
	}
#else
	if (parent_id > 0) {
		/* check whether folder is exist or not. */
		if (_get_exists_id(parent_id) <= 0) {
			FAVORITES_LOGE("Not found parent folder (%d)\n", parent_id);
			return FAVORITES_ERROR_NO_SUCH_FILE;
		}
	} else {
		parent_id = _get_root_folder_id();
	}
#endif
	FAVORITES_LOGD("[%s][%d]", title, parent_id);

	int uid = _get_exists_folder(title, parent_id);
	/* ignore this call.. already exist. */
	if (uid > 0) {
		FAVORITES_LOGE("Folder is already exist. cancel the add operation.");
		return FAVORITES_ERROR_ITEM_ALREADY_EXIST;
	}

	int lastIndex = _favorites_get_bookmark_lastindex(parent_id);
	if (lastIndex < 0) {
		FAVORITES_LOGE("Database::getLastIndex() is failed.\n");
		return FAVORITES_ERROR_DB_FAILED;
	}
	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	int nError;
	sqlite3_stmt *stmt;

	nError =
	    sqlite3_prepare_v2(gl_internet_bookmark_db,
					"INSERT INTO bookmarks (type, parent, title, creationdate,\
					updatedate, sequence, editable)\
					VALUES (1, ?, ?, DATETIME('now'), DATETIME('now'), ?, 1)",
			       -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}

	/*parent */
	if (sqlite3_bind_int(stmt, 1, parent_id) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_int is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	/*title */
	if (sqlite3_bind_text(stmt, 2, title, -1, NULL) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_text is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	/* order */
	if (lastIndex >= 0) {
		if (sqlite3_bind_int(stmt, 3, (lastIndex + 1)) != SQLITE_OK) {
			FAVORITES_LOGE("sqlite3_bind_int is failed.\n");
			_favorites_finalize_bookmark_db(stmt);
			return FAVORITES_ERROR_DB_FAILED;
		}
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_OK || nError == SQLITE_DONE) {
		_favorites_finalize_bookmark_db(stmt);
		int id = _get_folder_id(title, parent_id);
		if (id > 0)
			*saved_folder_id = id;
		return FAVORITES_ERROR_NONE;
	}
	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_close_bookmark_db();
	return FAVORITES_ERROR_DB_FAILED;
#endif
}


int _get_root_folder_id(void)
{
#if defined(BROWSER_BOOKMARK_SYNC)
	FAVORITES_LOGE("SYNC");
	int root_id = -1;
	bp_bookmark_adaptor_get_root(&root_id);
	return root_id;
#else
#if defined(ROOT_IS_ZERO)
	return 0;
#else
	FAVORITES_LOGE("NOT SYNC");
	return 1;
#endif
#endif
}

int _get_exists_id(int id)
{
	FAVORITES_LOGE("");
#if defined(BROWSER_BOOKMARK_SYNC)
	int value = -1;
	int ret = -1;
	int errorcode = -1;
	ret = bp_bookmark_adaptor_get_type(id, &value);
	if (ret < 0) {
		errorcode = bp_bookmark_adaptor_get_errorcode();
		if (errorcode == BP_BOOKMARK_ERROR_NO_DATA)
			return 0;
		else
			return -1;
	}
	return 1;
#else
	int nError;
	sqlite3_stmt *stmt;

	if (id < 0)
		return -1;

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return -1;
	}

	/* same title  */
	nError =
	    sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "select * from bookmarks where id=?",
			       -1, &stmt, NULL);

	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}
	if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_int is failed.");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}
	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		/* The given foldername is exist on the bookmark table */
		_favorites_finalize_bookmark_db(stmt);
		return 1;
	} else if (nError == SQLITE_DONE) {
		_favorites_finalize_bookmark_db(stmt);
		return 0;
	}

	_favorites_close_bookmark_db();
	return -1;
#endif
}

int _get_exists_bookmark(const char *address)
{
	FAVORITES_LOGD("");
	int nError;
	sqlite3_stmt *stmt;

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return -1;
	}
	/* same title  */
	nError =
	    sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "select * from bookmarks where address=? and type=0",
			       -1, &stmt, NULL);

	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	if (sqlite3_bind_text(stmt, 1, address, -1, NULL) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_text is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		_favorites_finalize_bookmark_db(stmt);
		return 1;
	} else if (nError == SQLITE_DONE) {
		//FAVORITES_LOGE("bookmark doesn't exists");
		_favorites_finalize_bookmark_db(stmt);
		return 0;
	}
	_favorites_close_bookmark_db();
	return -1;
}

int _get_exists_folder(const char *title, int parent_id)
{
	FAVORITES_LOGD("");
	int nError;
	sqlite3_stmt *stmt;

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return -1;
	}
	/* same title  */
	nError =
	    sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "select * from bookmarks where title=? and parent=? and type=1",
			       -1, &stmt, NULL);

	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	if (sqlite3_bind_text(stmt, 1, title, -1, NULL) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_text is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	if (sqlite3_bind_int(stmt, 2, parent_id) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_int is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		_favorites_finalize_bookmark_db(stmt);
		return 1;
	} else if (nError == SQLITE_DONE) {
		//FAVORITES_LOGE("folder doesn't exists");
		_favorites_finalize_bookmark_db(stmt);
		return 0;
	}
	_favorites_close_bookmark_db();
	return -1;
}

int _get_bookmark_id(const char *address)
{
	FAVORITES_LOGE("");
#if defined(BROWSER_BOOKMARK_SYNC)
	int id = -1;
	int *ids = NULL;
	int ids_count = -1;
	bp_bookmark_adaptor_get_duplicated_url_ids_p(&ids, &ids_count, 1,
		-1, -1, 0, 0, address, 0);
	if (ids_count > 0)
		id = ids[0];
	free(ids);

	if (ids_count > 0) {
		return id;
	}
	FAVORITES_LOGE("There is no such bookmark url exists.");
	return -1;
#else
	int nError;
	sqlite3_stmt *stmt;

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return -1;
	}
	/* same title  */
	nError =
	    sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "select id from bookmarks where address=? and type=0",
			       -1, &stmt, NULL);

	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	if (sqlite3_bind_text(stmt, 1, address, -1, NULL) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_text is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		int id = sqlite3_column_int(stmt, 0);
		_favorites_finalize_bookmark_db(stmt);
		return id;
	} else if (nError == SQLITE_DONE) {
		//FAVORITES_LOGE("bookmark id doesn't exists");
		_favorites_finalize_bookmark_db(stmt);
		return 0;
	}
	FAVORITES_LOGE("sqlite3_step is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
	_favorites_close_bookmark_db();
	return -1;
#endif
}

int _get_folder_id(const char *title, const int parent)
{
	FAVORITES_LOGE("");
#if defined(BROWSER_BOOKMARK_SYNC)
	int id = -1;
	int *ids = NULL;
	int ids_count = -1;
	bp_bookmark_adaptor_get_duplicated_title_ids_p(&ids, &ids_count, 1,
		-1, parent, 1, 0, title, 0);
	if (ids_count > 0)
		id = ids[0];
	free(ids);

	if (ids_count > 0) {
		return id;
	}
	FAVORITES_LOGE("There is no such folder exists.");
	return -1;
#else
	int nError;
	sqlite3_stmt *stmt;

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return -1;
	}
	/* same title  */
	nError =
	    sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "SELECT id FROM bookmarks WHERE title=? AND parent=? AND type=1",
			       -1, &stmt, NULL);

	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	if (sqlite3_bind_text(stmt, 1, title, -1, NULL) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_text is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}
	if (sqlite3_bind_int(stmt, 2, parent) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_int is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		int id = sqlite3_column_int(stmt, 0);
		_favorites_finalize_bookmark_db(stmt);
		return id;
	} else if (nError == SQLITE_DONE) {
		//FAVORITES_LOGE("folder id doesn't exists");
		_favorites_finalize_bookmark_db(stmt);
		return 0;
	}
	FAVORITES_LOGE("sqlite3_step is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
	_favorites_close_bookmark_db();
	return -1;
#endif
}

int _get_count_by_folder(int parent_id)
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
			       "select count(*) from bookmarks where parent=?",
			       -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	if (sqlite3_bind_int(stmt, 1, parent_id) != SQLITE_OK) {
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
	} else if (nError == SQLITE_DONE) {
		FAVORITES_LOGE("No rows");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}
	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_close_bookmark_db();
	return -1;
}


int _get_list_by_folder(int parent_id, bookmark_list_h *list)
{
	FAVORITES_LOGD("");
#if defined(BROWSER_BOOKMARK_SYNC)
	int *ids = NULL;
	int ids_count = -1;
	bp_bookmark_info_fmt info;
	if (bp_bookmark_adaptor_get_sequence_child_ids_p(&ids, &ids_count, -1, 0, parent_id, -1, 0) < 0) {
		FAVORITES_LOGE("bp_bookmark_adaptor_get_sequence_child_ids_p is failed");
		return FAVORITES_ERROR_DB_FAILED;
	}

	FAVORITES_LOGD("ids_count: %d", ids_count);
	if (ids_count < 0) {
		FAVORITES_LOGE("bookmark list is empty");
		return FAVORITES_ERROR_DB_FAILED;
	}

	*list = (bookmark_list_h) calloc(1, sizeof(bookmark_list_s));
	(*list)->count = 0;
	(*list)->item =
	    (bookmark_entry_internal_h) calloc(ids_count, sizeof(bookmark_entry_internal_s));
	int i = 0;
	for(i = 0; i < ids_count; i++) {
		(*list)->item[i].id = ids[i];
		if (bp_bookmark_adaptor_get_info(ids[i], (BP_BOOKMARK_O_TYPE |
				BP_BOOKMARK_O_PARENT | BP_BOOKMARK_O_SEQUENCE |
				BP_BOOKMARK_O_IS_EDITABLE | BP_BOOKMARK_O_URL |
				BP_BOOKMARK_O_TITLE | BP_BOOKMARK_O_DATE_CREATED |
				BP_BOOKMARK_O_DATE_MODIFIED), &info) == 0) {
			(*list)->item[i].is_folder = info.type;
			(*list)->item[i].folder_id = info.parent;
			(*list)->item[i].orderIndex = info.sequence;
			(*list)->item[i].editable = info.editable;

			if (info.url != NULL && strlen(info.url) > 0)
				(*list)->item[i].address = strdup(info.url);
			if (info.title != NULL && strlen(info.title) > 0)
				(*list)->item[i].title = strdup(info.title);
			FAVORITES_LOGD("Title: %s", (*list)->item[i].title);

			(*list)->item[i].creationdate = NULL;
			(*list)->item[i].creationdate = (char *)calloc(128, sizeof(char));
			strftime((*list)->item[i].creationdate,128,"%Y-%m-%d %H:%M:%S",localtime((time_t *)&(info.date_created)));

			(*list)->item[i].updatedate = NULL;
			(*list)->item[i].updatedate = (char *)calloc(128, sizeof(char));
			strftime((*list)->item[i].updatedate,128,"%Y-%m-%d %H:%M:%S",localtime((time_t *)&(info.date_modified)));
		}
		bp_bookmark_adaptor_easy_free(&info);
	}
	(*list)->count = i;
	FAVORITES_LOGE("bookmark list count : %d", (*list)->count);
	free(ids);
	return FAVORITES_ERROR_NONE;
#else
	int nError;
	sqlite3_stmt *stmt;

	int item_count = _get_count_by_folder(parent_id);
	if (item_count < 0) {
		FAVORITES_LOGD("_get_count_by_folder is failed");
		return FAVORITES_ERROR_DB_FAILED;
	}

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "select id, type, parent, address, title, editable,\
			       creationdate, updatedate, sequence \
			       from bookmarks where parent=? order by sequence",
			       -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}

	if (sqlite3_bind_int(stmt, 1, parent_id) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_int is failed.\n");
		_favorites_finalize_bookmark_db(stmt);
		return -1;
	}

	*list = (bookmark_list_h) calloc(1, sizeof(bookmark_list_s));
	(*list)->count = 0;
	(*list)->item =
	    (bookmark_entry_internal_h) calloc(item_count, sizeof(bookmark_entry_internal_s));
	int i = 0;
	while ((nError = sqlite3_step(stmt)) == SQLITE_ROW && (i < item_count)) {
		(*list)->item[i].id = sqlite3_column_int(stmt, 0);
		(*list)->item[i].is_folder = sqlite3_column_int(stmt, 1);
		(*list)->item[i].folder_id = sqlite3_column_int(stmt, 2);

		(*list)->item[i].address = NULL;
		if (!(*list)->item[i].is_folder) {
			const char *url = (const char *)(sqlite3_column_text(stmt, 3));
			if (url) {
				int length = strlen(url);
				if (length > 0) {
					(*list)->item[i].address = (char *)calloc(length + 1, sizeof(char));
					memcpy((*list)->item[i].address, url, length);
					FAVORITES_LOGE ("url:%s\n", url);
				}
			}
		}

		const char *title = (const char *)(sqlite3_column_text(stmt, 4));
		(*list)->item[i].title = NULL;
		if (title) {
			int length = strlen(title);
			if (length > 0) {
				(*list)->item[i].title = (char *)calloc(length + 1, sizeof(char));
				memcpy((*list)->item[i].title, title, length);
			}
		}
		(*list)->item[i].editable = sqlite3_column_int(stmt, 5);

		const char *creationdate = (const char *)(sqlite3_column_text(stmt, 6));
		(*list)->item[i].creationdate = NULL;
		if (creationdate) {
			int length = strlen(creationdate);
			if (length > 0) {
				(*list)->item[i].creationdate = (char *)calloc(length + 1, sizeof(char));
				memcpy((*list)->item[i].creationdate, creationdate, length);
			}
		}
		const char *updatedate = (const char *)(sqlite3_column_text(stmt, 7));
		(*list)->item[i].updatedate = NULL;
		if (updatedate) {
			int length = strlen(updatedate);
			if (length > 0) {
				(*list)->item[i].updatedate = (char *)calloc(length + 1, sizeof(char));
				memcpy((*list)->item[i].updatedate, updatedate, length);
			}
		}

		(*list)->item[i].orderIndex = sqlite3_column_int(stmt, 8);
		i++;
	}

	(*list)->count = i;
	FAVORITES_LOGE("bookmark list count : %d", (*list)->count);

	if ((nError != SQLITE_ROW) && (nError != SQLITE_DONE)) {
		FAVORITES_LOGE("sqlite3_step is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}

	_favorites_finalize_bookmark_db(stmt);
	return FAVORITES_ERROR_NONE;
#endif
}

int _set_full_tree_to_html_recur(int parent_id, FILE *fp, int depth)
{
	FAVORITES_LOGD("depth: %d", depth);
	if (parent_id < 0)
		return FAVORITES_ERROR_INVALID_PARAMETER;
	if (fp == NULL)
		return FAVORITES_ERROR_INVALID_PARAMETER;
	if (depth < 0)
		return FAVORITES_ERROR_INVALID_PARAMETER;

	bookmark_list_h child_list = NULL;

	int ret = _get_list_by_folder(parent_id, &child_list);

	if (ret != FAVORITES_ERROR_NONE) {
		_favorites_free_bookmark_list(child_list);
		return FAVORITES_ERROR_DB_FAILED;
	}

	int i = 0;
	int j = 0;
	FAVORITES_LOGD("list->count: %d", child_list->count);
	for (i=0; i < (child_list->count); i++) {
		FAVORITES_LOGD("title: %s", child_list->item[i].title);
		if (child_list->item[i].is_folder) {
			for (j=0 ; j < depth ; j++) {
				fputs("\t", fp);
			}
			int folder_adddate_unixtime = 0;
			folder_adddate_unixtime =
				_favorites_get_unixtime_from_datetime(
					child_list->item[i].creationdate);
			fprintf(fp, "<DT><H3 FOLDED ADD_DATE=\"%d\">%s</H3>\n",
					folder_adddate_unixtime, child_list->item[i].title);
			for (j=0 ; j < depth ; j++) {
				fputs("\t", fp);
			}
			fputs("<DL><p>\n", fp);
			ret = _set_full_tree_to_html_recur(child_list->item[i].id, fp, depth+1);
			if (ret != FAVORITES_ERROR_NONE) {
				_favorites_free_bookmark_list(child_list);
				return ret;
			}
			for (j=0 ; j < depth ; j++) {
				fputs("\t", fp);
			}
			fputs("</DL><p>\n", fp);
		}else {
			int bookmark_adddate_unixtime =
				_favorites_get_unixtime_from_datetime(
				child_list->item[i].creationdate);

			if(bookmark_adddate_unixtime<0)
				bookmark_adddate_unixtime=0;

			int bookmark_updatedate_unixtime =
				_favorites_get_unixtime_from_datetime(
				child_list->item[i].updatedate);

			if(bookmark_updatedate_unixtime<0)
				bookmark_updatedate_unixtime=0;

			for (j=0 ; j < depth ; j++) {
				fputs("\t", fp);
			}
			fprintf(fp,"<DT><A HREF=\"%s\" ",
					child_list->item[i].address);
			fprintf(fp,"ADD_DATE=\"%d\" ",
					bookmark_adddate_unixtime);
			fprintf(fp,"LAST_VISIT=\"%d\" ",
					bookmark_updatedate_unixtime);
			fprintf(fp,"LAST_MODIFIED=\"%d\">",
					bookmark_updatedate_unixtime);
			fprintf(fp,"%s</A>\n", child_list->item[i].title );
		}
	}

	_favorites_free_bookmark_list(child_list);
	return FAVORITES_ERROR_NONE;
}

/*************************************************************
 *	APIs for Internet favorites
 *************************************************************/
int favorites_bookmark_get_root_folder_id(int *root_id)
{
	FAVORITES_LOGE("");
	if (!root_id)
		return FAVORITES_ERROR_INVALID_PARAMETER;

	*root_id = _get_root_folder_id();
	FAVORITES_LOGE("root_id: %d", *root_id);
	return FAVORITES_ERROR_NONE;
}

int favorites_bookmark_add_bookmark(const char *url, const char *title, const char *foldername, int *bookmark_id)
{
	FAVORITES_LOGE("");
	int ret = 0;
	int root_id = 1;
	int folder_id = 1;

	if (!foldername || (strlen(foldername) <= 0)) {
		FAVORITES_LOGE("foldername is empty. id is now root.\n");
		folder_id = _get_root_folder_id();
	} else {
		FAVORITES_LOGE("adding folder\n");
		ret = favorites_bookmark_add(foldername, url, root_id, 1, &folder_id);
		if (ret != FAVORITES_ERROR_NONE) {
			if (ret == FAVORITES_ERROR_ITEM_ALREADY_EXIST) {
				folder_id = _get_folder_id(foldername, _get_root_folder_id());
			} else
				return ret;
		}
	}
	return favorites_bookmark_add(title, url, folder_id, 0, bookmark_id);
}

int favorites_bookmark_add(const char *title, const char *url, int parent_id, int type, int *saved_id)
{
	FAVORITES_LOGD("");
	int ret = 0;
	if (type == 0) {
		FAVORITES_LOGD("type is bookmark");
		ret = _add_bookmark(title, url, parent_id, saved_id);
	} else {
		FAVORITES_LOGD("type is folder");
		ret = _add_folder(title, parent_id, saved_id);
	}

	return ret;
}

int favorites_bookmark_get_count(int *count)
{
	FAVORITES_LOGD("");
#if defined(BROWSER_BOOKMARK_SYNC)
	int ids_count = 0;
	int *ids = NULL;
	bp_bookmark_adaptor_get_full_ids_p(&ids, &ids_count);
	*count = ids_count;
	free(ids);

	if (*count <0) {
		FAVORITES_LOGE("bp_bookmark_adaptor_get_full_ids_p is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}
	return FAVORITES_ERROR_NONE;
#else
	int nError;
	sqlite3_stmt *stmt;

	FAVORITES_NULL_ARG_CHECK(count);
	
	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	/* folder + bookmark */
#if defined(ROOT_IS_ZERO)
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "select count(*) from bookmarks",
			       -1, &stmt, NULL);
#else
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "select count(*) from bookmarks where parent != 0",
			       -1, &stmt, NULL);
#endif
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		*count = sqlite3_column_int(stmt, 0);
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_NONE;
	} else if (nError == SQLITE_DONE) {
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_NONE;
	}
	_favorites_close_bookmark_db();
	return FAVORITES_ERROR_DB_FAILED;
#endif
}

int favorites_bookmark_foreach(favorites_bookmark_foreach_cb callback,void *user_data)
{
	FAVORITES_NULL_ARG_CHECK(callback);
#if defined(BROWSER_BOOKMARK_SYNC)
	int ids_count = 0;
	int *ids = NULL;
	int func_ret = 0;
	int is_folder = -1;
	int editable = -1;

	if (bp_bookmark_adaptor_get_full_ids_p(&ids, &ids_count) < 0) {
		FAVORITES_LOGE ("bp_bookmark_adaptor_get_full_ids_p is failed.\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	FAVORITES_LOGD("bp_bookmark_adaptor_get_full_ids_p count : %d", ids_count);
	if (ids_count > 0) {
		int i = 0;
		for (i = 0; i < ids_count; i++) {
			FAVORITES_LOGD("I[%d] id : %d", i, ids[i]);
			favorites_bookmark_entry_s result;
			memset(&result, 0x00, sizeof(favorites_bookmark_entry_s));
			result.id = ids[i];
			bp_bookmark_adaptor_get_type(result.id, &is_folder);
			if (is_folder > 0)
				result.is_folder = 1;
			else
				result.is_folder = 0;
			FAVORITES_LOGD("is_folder : %d", result.is_folder);
			bp_bookmark_adaptor_get_parent_id(result.id, &(result.folder_id));
			FAVORITES_LOGD("folder_id : %d", result.folder_id);
			bp_bookmark_adaptor_get_url(result.id, &(result.address));
			FAVORITES_LOGD("address : %s", result.address);
			bp_bookmark_adaptor_get_title(result.id, &(result.title));
			FAVORITES_LOGD("title : %s", result.title);
			bp_bookmark_adaptor_get_is_editable(result.id, &editable);
			if (editable > 0)
				result.editable = 1;
			else
				result.editable = 0;
			bp_bookmark_adaptor_get_sequence(result.id, &(result.order_index));
			FAVORITES_LOGD("order_index : %d", result.order_index);

			result.creation_date = NULL;
			int recv_int = -1;
			bp_bookmark_adaptor_get_date_created(result.id, &recv_int);
			result.creation_date = (char *)calloc(128, sizeof(char));
			strftime(result.creation_date,128,"%Y-%m-%d %H:%M:%S",localtime((time_t *)&recv_int));
			FAVORITES_LOGD("Creationdata:%s", result.creation_date);

			result.update_date = NULL;
			bp_bookmark_adaptor_get_date_modified(result.id, &recv_int);
			result.update_date = (char *)calloc(128, sizeof(char));
			strftime(result.update_date,128,"%Y-%m-%d %H:%M:%S",localtime((time_t *)&recv_int));
			FAVORITES_LOGD("update_date:%s", result.update_date);

			result.visit_date = NULL;
			bp_bookmark_adaptor_get_date_visited(result.id, &recv_int);
			result.visit_date = (char *)calloc(128, sizeof(char));
			strftime(result.visit_date,128,"%Y-%m-%d %H:%M:%S",localtime((time_t *)&recv_int));
			FAVORITES_LOGD("visit_date:%s", result.visit_date);

			func_ret = callback(&result, user_data);
			_favorites_free_bookmark_entry(&result);
			if(func_ret == 0) 
				break;
		}
	}
	free(ids);
	FAVORITES_LOGE ("There are no more bookmarks.\n");
	return FAVORITES_ERROR_NONE;
#else
	int nError;
	int func_ret = 0;
	sqlite3_stmt *stmt;

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}
#if defined(ROOT_IS_ZERO)
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "select id, type, parent, address, title, editable,\
			       creationdate, updatedate, sequence \
			       from bookmarks order by sequence",
			       -1, &stmt, NULL);
#else
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			       "select id, type, parent, address, title, editable,\
			       creationdate, updatedate, sequence \
			       from bookmarks where parent != 0 order by sequence",
			       -1, &stmt, NULL);
#endif
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
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
				FAVORITES_LOGE ("title:%s\n", title);
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
#endif
}

int favorites_bookmark_export_list(const char *file_path)
{
	FAVORITES_NULL_ARG_CHECK(file_path);

	int root_id = -1;
	favorites_bookmark_get_root_folder_id(&root_id);

	FILE *fp = NULL;
	FAVORITES_LOGE("file_path: %s", file_path);
	fp = fopen(file_path, "w");
	if (fp == NULL) {
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

	int ret = -1;
	ret = _set_full_tree_to_html_recur(root_id, fp, 1);
	if (ret != FAVORITES_ERROR_NONE) {
		fclose(fp);
		return FAVORITES_ERROR_DB_FAILED;
	}

	fputs("</DL><p>\n", fp);
	fclose(fp);
	return FAVORITES_ERROR_NONE;
}

int favorites_bookmark_get_favicon(int id, Evas *evas, Evas_Object **icon)
{
	FAVORITES_LOGD("");
	FAVORITES_INVALID_ARG_CHECK(id<0);
	FAVORITES_NULL_ARG_CHECK(evas);
	FAVORITES_NULL_ARG_CHECK(icon);
#if defined(BROWSER_BOOKMARK_SYNC)
	void *favicon_data_temp=NULL;
	favicon_entry_h favicon;
	favicon = (favicon_entry_h) calloc(1, sizeof(favicon_entry_s));
	int ret = bp_bookmark_adaptor_get_favicon(id, (unsigned char **)&favicon_data_temp, &(favicon->length));
	if (ret == 0) {
		if (bp_bookmark_adaptor_get_favicon_width(id, &(favicon->w)) == 0) {
			if (bp_bookmark_adaptor_get_favicon_height(id, &(favicon->h)) == 0) {
				FAVORITES_LOGD("favicon is successfully loaded.");
			} else {
				FAVORITES_LOGE("bp_bookmark_adaptor_set_favicon_height is failed");
				*icon = NULL;
				return FAVORITES_ERROR_DB_FAILED;
			}
		} else {
			FAVORITES_LOGE("bp_bookmark_adaptor_set_favicon_width is failed");
			*icon = NULL;
			return FAVORITES_ERROR_DB_FAILED;
		}
	} else {
		FAVORITES_LOGE("bp_bookmark_adaptor_set_favicon is failed");
		*icon = NULL;
		return FAVORITES_ERROR_DB_FAILED;
	}
	FAVORITES_LOGD("len: %d, w:%d, h:%d", favicon->length, favicon->w, favicon->h);
	if (favicon->length > 0){
		*icon = evas_object_image_filled_add(evas);
		evas_object_image_colorspace_set(*icon, EVAS_COLORSPACE_ARGB8888);
		evas_object_image_size_set(*icon, favicon->w, favicon->h);
		evas_object_image_fill_set(*icon, 0, 0, favicon->w, favicon->h);
		evas_object_image_filled_set(*icon, EINA_TRUE);
		evas_object_image_alpha_set(*icon,EINA_TRUE);
		evas_object_image_data_set(*icon, favicon_data_temp);
		return FAVORITES_ERROR_NONE;
	}
	*icon = NULL;
	FAVORITES_LOGE("favicon length is 0");
	return FAVORITES_ERROR_DB_FAILED;
#else
	void *favicon_data_temp=NULL;
	favicon_entry_h favicon;
	sqlite3_stmt *stmt;
	char	query[1024];
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
		free(favicon);
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_NONE;
	}

	_favorites_close_bookmark_db();
	return FAVORITES_ERROR_NONE;
#endif
}

int favorites_bookmark_set_favicon(int bookmark_id, Evas_Object *icon)
{
	FAVORITES_LOGD("");
	FAVORITES_INVALID_ARG_CHECK(bookmark_id<0);
	FAVORITES_NULL_ARG_CHECK(icon);
#if defined(BROWSER_BOOKMARK_SYNC)
	int icon_w = 0;
	int icon_h = 0;
	int stride = 0;
	int icon_length = 0;
	void *icon_data = (void *)evas_object_image_data_get(icon, EINA_TRUE);
	evas_object_image_size_get(icon, &icon_w, &icon_h);
	stride = evas_object_image_stride_get(icon);
	icon_length = icon_h * stride;
	FAVORITES_LOGD("favicon size  w:%d, h:%d, stride: %d", icon_w, icon_h, stride);
	int ret = bp_bookmark_adaptor_set_favicon(bookmark_id,(const unsigned char *)icon_data, icon_length);
	if (ret == 0) {
		if (bp_bookmark_adaptor_set_favicon_width(bookmark_id, icon_w) == 0) {
			if (bp_bookmark_adaptor_set_favicon_height(bookmark_id, icon_h) == 0) {
				FAVORITES_LOGE("favicon is successfully saved.");
				bp_bookmark_adaptor_publish_notification();
				return FAVORITES_ERROR_NONE;
			} else
				FAVORITES_LOGE("bp_bookmark_adaptor_set_favicon_height is failed");
		} else
			FAVORITES_LOGE("bp_bookmark_adaptor_set_favicon_width is failed");
	} else
		FAVORITES_LOGE("bp_bookmark_adaptor_set_favicon is failed");
	FAVORITES_LOGE("set favicon is failed");
	return FAVORITES_ERROR_DB_FAILED;
#else
	int icon_w = 0;
	int icon_h = 0;
	int stride = 0;
	int icon_length = 0;
	void *icon_data = (void *)evas_object_image_data_get(icon, EINA_TRUE);
	evas_object_image_size_get(icon, &icon_w, &icon_h);
	stride = evas_object_image_stride_get(icon);
	icon_length = icon_h * stride;
	FAVORITES_LOGD("favicon size  w:%d, h:%d, stride: %d", icon_w, icon_h, stride);
	int nError;
	sqlite3_stmt *stmt;

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
				"UPDATE bookmarks SET favicon=?,\
				favicon_length=?, favicon_w=?, favicon_h=? \
				WHERE id=?", -1, &stmt, NULL);

	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}

	// bind
	if (sqlite3_bind_blob(stmt, 1, icon_data , icon_length, NULL) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_blob(icon_data) is failed");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	if (sqlite3_bind_int(stmt, 2, icon_length) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_int(icon_length) is failed");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	if (sqlite3_bind_int(stmt, 3, icon_w) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_int(icon_w) is failed");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	if (sqlite3_bind_int(stmt, 4, icon_h) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_int(icon_h) is failed");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	if (sqlite3_bind_int(stmt, 5, bookmark_id) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_int(bookmark_id) is failed");
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW || nError == SQLITE_DONE) {
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_NONE;
	}

	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_close_bookmark_db();
	return FAVORITES_ERROR_DB_FAILED;
#endif
}

int favorites_bookmark_delete_bookmark(int id)
{
	FAVORITES_LOGE("");
	FAVORITES_INVALID_ARG_CHECK(id<0);
#if defined(BROWSER_BOOKMARK_SYNC)
	if (bp_bookmark_adaptor_delete(id) < 0)
		return FAVORITES_ERROR_DB_FAILED;
	else {
		bp_bookmark_adaptor_publish_notification();
		return FAVORITES_ERROR_NONE;
	}
#else
	int nError;
	sqlite3_stmt *stmt;

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

#if defined(ROOT_IS_ZERO)
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			        "delete from bookmarks where id=?", -1, &stmt, NULL);
#else
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			        "delete from bookmarks where id=? and parent != 0", -1, &stmt, NULL);
#endif
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
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
	if (nError == SQLITE_ROW || nError == SQLITE_DONE) {
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_NONE;
	}
	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_close_bookmark_db();
	return FAVORITES_ERROR_DB_FAILED;
#endif
}

int favorites_bookmark_delete_all_bookmarks(void)
{
	FAVORITES_LOGD("");
#if defined(BROWSER_BOOKMARK_SYNC)
	int ids_count = 0;
	int *ids = NULL;

	if (bp_bookmark_adaptor_get_full_ids_p(&ids, &ids_count) < 0) {
		FAVORITES_LOGE ("bp_bookmark_adaptor_get_full_ids_p is failed.\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	FAVORITES_LOGD("bp_bookmark_adaptor_get_full_ids_p count : %d", ids_count);
	if (ids_count > 0) {
		int i = 0;
		for (i = 0; i < ids_count; i++) {
			FAVORITES_LOGD("I[%d] id : %d", i, ids[i]);
			if (bp_bookmark_adaptor_delete(ids[i]) < 0) {
				free(ids);
				return FAVORITES_ERROR_DB_FAILED;
			} else
				bp_bookmark_adaptor_publish_notification();
		}
	}
	free(ids);
	return FAVORITES_ERROR_NONE;
#else
	int nError;
	sqlite3_stmt *stmt;

	if (_favorites_open_bookmark_db() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

#if defined(ROOT_IS_ZERO)
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			        "delete from bookmarks", -1, &stmt, NULL);
#else
	nError = sqlite3_prepare_v2(gl_internet_bookmark_db,
			        "delete from bookmarks where parent !=0", -1, &stmt, NULL);
#endif
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed(%s).\n",
			sqlite3_errmsg(gl_internet_bookmark_db));
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW || nError == SQLITE_DONE) {
		_favorites_finalize_bookmark_db(stmt);
		return FAVORITES_ERROR_NONE;
	}
	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_close_bookmark_db();
	return FAVORITES_ERROR_DB_FAILED;
#endif
}

