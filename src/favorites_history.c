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

__thread sqlite3 *gl_internet_history_db = 0;

#define INTERNET_HISTORY_DB_NAME "/opt/dbspace/.browser-history.db"

/* Private Functions */
void _favorites_history_db_close(void)
{
	if (gl_internet_history_db) {
		/* ASSERT(currentThread() == m_openingThread); */
		db_util_close(gl_internet_history_db);
		gl_internet_history_db = 0;
	}
}

void _favorites_history_db_finalize(sqlite3_stmt *stmt)
{
	if (sqlite3_finalize(stmt) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_finalize is failed");
	}
	_favorites_history_db_close();
}
int _favorites_history_db_open(void)
{
	_favorites_history_db_close();
	if (db_util_open
	    (INTERNET_HISTORY_DB_NAME, &gl_internet_history_db,
	     DB_UTIL_REGISTER_HOOK_METHOD) != SQLITE_OK) {
		db_util_close(gl_internet_history_db);
		gl_internet_history_db = 0;
		return -1;
	}
	return gl_internet_history_db ? 0 : -1;
}

int _favorites_free_history_entry(favorites_history_entry_s *entry)
{
	FAVORITES_NULL_ARG_CHECK(entry);

	if (entry->address != NULL)
		free(entry->address);
	if (entry->title != NULL)
		free(entry->title);
	if (entry->visit_date != NULL)
		free(entry->visit_date);

	return FAVORITES_ERROR_NONE;
}
/*************************************************************
 *	APIs for Internet favorites
 *************************************************************/
int favorites_history_get_count(int *count)
{
	int nError;
	sqlite3_stmt *stmt;
	FAVORITES_LOGE("");
		
	if (_favorites_history_db_open() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	/* folder + bookmark */
	nError = sqlite3_prepare_v2(gl_internet_history_db,
			       "select count(*) from history",
			       -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_history_db_finalize(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_ROW) {
		*count = sqlite3_column_int(stmt, 0);
		_favorites_history_db_finalize(stmt);
		return FAVORITES_ERROR_NONE;
	}
	_favorites_history_db_close();
	return FAVORITES_ERROR_DB_FAILED;
}
/* Public CAPI */
int favorites_history_foreach(favorites_history_foreach_cb callback,void *user_data)
{
	FAVORITES_NULL_ARG_CHECK(callback);
	int nError;
	int func_ret = 0;
	sqlite3_stmt *stmt;

	if (_favorites_history_db_open() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}
	nError = sqlite3_prepare_v2(gl_internet_history_db,
			       "select id, address, title, counter, visitdate\
			       from history order by visitdate desc",
			       -1, &stmt, NULL);

	if (nError != SQLITE_OK) {
		FAVORITES_LOGE ("sqlite3_prepare_v2 is failed.\n");
		_favorites_history_db_finalize(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}

	while ((nError = sqlite3_step(stmt)) == SQLITE_ROW) {
		favorites_history_entry_s result;
		result.id = sqlite3_column_int(stmt, 0);

		result.address = NULL;
		const char *url = (const char *)(sqlite3_column_text(stmt, 1));
		if (url) {
			int length = strlen(url);
			if (length > 0) {
				result.address = (char *)calloc(length + 1, sizeof(char));
				memcpy(result.address, url, length);
				FAVORITES_LOGE ("url:%s\n", url);
			}
		}

		const char *title = (const char *)(sqlite3_column_text(stmt, 2));
		result.title = NULL;
		if (title) {
			int length = strlen(title);
			if (length > 0) {
				result.title = (char *)calloc(length + 1, sizeof(char));
				memcpy(result.title, title, length);
			}
		}
		result.count = sqlite3_column_int(stmt, 3);

		const char *visit_date =
		    (const char *)(sqlite3_column_text(stmt, 4));
		result.visit_date = NULL;
		if (visit_date) {
			int length = strlen(visit_date);
			if (length > 0) {
				result.visit_date =
				    (char *)calloc(length + 1, sizeof(char));
				memcpy(result.visit_date,
				       visit_date, length);
				FAVORITES_LOGE("Date:%s\n", result.visit_date);
			}
		}

		func_ret = callback(&result, user_data);
		_favorites_free_history_entry(&result);
		if(func_ret == 0) 
			break;
	}

	_favorites_history_db_finalize(stmt);
	return FAVORITES_ERROR_NONE;
}

int favorites_history_delete_history(int id)
{
	int nError;
	sqlite3_stmt *stmt;

	FAVORITES_INVALID_ARG_CHECK(id<0);

	if (_favorites_history_db_open() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_prepare_v2(gl_internet_history_db,
			        "delete from history where id=?", -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_history_db_finalize(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	// bind
	if (sqlite3_bind_int(stmt, 1, id) != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_bind_int is failed");
		_favorites_history_db_finalize(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	nError = sqlite3_step(stmt);
	if (nError == SQLITE_OK || nError == SQLITE_DONE) {
		_favorites_history_db_finalize(stmt);
		return FAVORITES_ERROR_NONE;
	}
	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_history_db_close();
	return FAVORITES_ERROR_DB_FAILED;
}

int favorites_history_delete_history_by_url(const char *url)
{
	int nError;
	sqlite3_stmt *stmt;

	if (!url || (strlen(url) <= 0)) {
		FAVORITES_LOGE("url is empty\n");
		return FAVORITES_ERROR_INVALID_PARAMETER;
	}
		
	if (_favorites_history_db_open() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_prepare_v2(gl_internet_history_db,
			        "delete from history where address=?", -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_history_db_finalize(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}
	// bind
	if (sqlite3_bind_text(stmt, 1, url, -1, NULL) != SQLITE_OK)
		FAVORITES_LOGE("sqlite3_bind_text is failed.\n");
	
	nError = sqlite3_step(stmt);
	if (nError == SQLITE_OK || nError == SQLITE_DONE) {
		_favorites_history_db_finalize(stmt);
		return FAVORITES_ERROR_NONE;
	}
	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_history_db_close();
	return FAVORITES_ERROR_DB_FAILED;
}

int favorites_history_delete_all_histories(void)
{
	int nError;
	sqlite3_stmt *stmt;

	if (_favorites_history_db_open() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_prepare_v2(gl_internet_history_db,
			        "delete from history", -1, &stmt, NULL);
	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_history_db_finalize(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_OK || nError == SQLITE_DONE) {
		_favorites_history_db_finalize(stmt);
		return FAVORITES_ERROR_NONE;
	}
	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_history_db_close();
	return FAVORITES_ERROR_DB_FAILED;
}

int favorites_history_delete_history_by_term(const char *begin, const char *end)
{
	int nError;
	sqlite3_stmt *stmt;
	char	query[1024];

	memset(&query, 0x00, sizeof(char)*1024);

	if (!begin || (strlen(begin) <= 0)) {
		FAVORITES_LOGE("begin date is empty\n");
		return FAVORITES_ERROR_INVALID_PARAMETER;
	}

	if (!end || (strlen(end) <= 0)) {
		FAVORITES_LOGE("end date is empty\n");
		end = "now";
	}

	sprintf(query, "delete from history where visitdate\
		between datetime('%s') and datetime('%s')", begin, end);
		
	if (_favorites_history_db_open() < 0) {
		FAVORITES_LOGE("db_util_open is failed\n");
		return FAVORITES_ERROR_DB_FAILED;
	}
	nError = sqlite3_prepare_v2(gl_internet_history_db,
			        query, -1, &stmt, NULL);

	if (nError != SQLITE_OK) {
		FAVORITES_LOGE("sqlite3_prepare_v2 is failed.\n");
		_favorites_history_db_finalize(stmt);
		return FAVORITES_ERROR_DB_FAILED;
	}

	nError = sqlite3_step(stmt);
	if (nError == SQLITE_OK || nError == SQLITE_DONE) {
		FAVORITES_LOGE("sqlite3_step is DONE");
		_favorites_history_db_finalize(stmt);
		return FAVORITES_ERROR_NONE;
	}
	FAVORITES_LOGE("sqlite3_step is failed");
	_favorites_history_db_close();

	return FAVORITES_ERROR_DB_FAILED;
}

