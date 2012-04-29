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

#ifndef __TIZEN_WEB_FAVORITES_PRIVATE_H__
#define __TIZEN_WEB_FAVORITES_PRIVATE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************
 * Logging macro
 **********************************************/
#define FAVORITES_LOGD(fmt, args...) LOGD(\
		"[%s: %s: %d] "fmt, (rindex(__FILE__, '/')? rindex(__FILE__, '/') + 1 : __FILE__),\
		__FUNCTION__, __LINE__, ##args)
#define FAVORITES_LOGI(fmt, args...) LOGI(\
		"[%s: %s: %d] "fmt, (rindex(__FILE__, '/') ? rindex(__FILE__, '/') + 1 : __FILE__),\
		__FUNCTION__, __LINE__, ##args)
#define FAVORITES_LOGW(fmt, args...) LOGW(\
		"[%s: %s: %d] "fmt, (rindex(__FILE__, '/') ? rindex(__FILE__, '/') + 1 : __FILE__),\
		__FUNCTION__, __LINE__, ##args)
#define FAVORITES_LOGE(fmt, args...) LOGE(\
		"[%s: %s: %d] "fmt, (rindex(__FILE__, '/') ? rindex(__FILE__, '/') + 1 : __FILE__),\
		__FUNCTION__, __LINE__, ##args)
#define FAVORITES_LOGE_IF(cond, fmt, args...) LOGE_IF(cond,\
		"[%s: %s: %d] "fmt, (rindex(__FILE__, '/') ? rindex(__FILE__, '/') + 1 : __FILE__),\
		__FUNCTION__, __LINE__, ##args)

/**********************************************
 * Argument checking macro
 **********************************************/
#define FAVORITES_NULL_ARG_CHECK(_arg_)	do { \
	if(_arg_ == NULL) { \
        LOGE("[%s] FAVORITES_ERR_INVALID_PARAMETER(0x%08x)", __FUNCTION__,\
        FAVORITES_ERROR_INVALID_PARAMETER); \
        return FAVORITES_ERROR_INVALID_PARAMETER; \
    } \
}while(0)

#define FAVORITES_INVALID_ARG_CHECK(_condition_)	do { \
	if(_condition_) { \
        LOGE("[%s] FAVORITES_ERR_INVALID_PARAMETER(0x%08x)", __FUNCTION__,\
        FAVORITES_ERROR_INVALID_PARAMETER); \
        return FAVORITES_ERROR_INVALID_PARAMETER; \
    } \
}while(0)

#define _FAVORITES_FREE(_srcx_) 	{	if(NULL != _srcx_) free(_srcx_);	}
#define _FAVORITES_STRDUP(_srcx_) 	(NULL != _srcx_) ? strdup(_srcx_):NULL

struct bookmark_entry_internal{
	char* address;		/**< URL of the bookmark */
	char* title;			/**< Title of the bookmark */
	char* creationdate;	/**< date of created */
	char* updatedate;	/**< date of last updated */
	char* visitdate;		/**< date of last visited */
	int id;				/**< uniq id of bookmark */
	int is_folder;			/**< property bookmark or folder 1: bookmark 1: folder */
	int folder_id;			/**< parent folder id */
	int orderIndex;		/**< order sequence */
	int editable;			/**< flag of editability 1 : WRITABLE   0: READ ONLY */
};
typedef struct bookmark_entry_internal bookmark_entry_internal_s;
typedef struct bookmark_entry_internal *bookmark_entry_internal_h;

struct bookmark_list {
	int count;
	bookmark_entry_internal_h item;
};

/**
 * @brief The bookmark entry list handle.
 */
typedef struct bookmark_list bookmark_list_s;

/**
 * @brief The bookmark entry list structure.
 */
typedef struct bookmark_list *bookmark_list_h;

struct favicon_entry {
	void *data;	/* favicon image data pointer. ( Allocated memory) */
	int length;	/* favicon image data's length */
	int w;		/* favicon image width */
	int h;		/* favicon image height */
};
typedef struct favicon_entry favicon_entry_s;
typedef struct favicon_entry *favicon_entry_h;

/* bookmark internal API */
void _favorites_close_bookmark_db(void);
void _favorites_finalize_bookmark_db(sqlite3_stmt *stmt);
const char *_favorites_get_bookmark_db_name(void);
int _favorites_open_bookmark_db(void);
void _favorites_free_bookmark_list(bookmark_list_h m_list);
int _favorites_free_bookmark_entry(favorites_bookmark_entry_s *entry);
int _favorites_get_bookmark_lastindex(int locationId);
int _favorites_bookmark_get_folderID(const char *foldername);
int _favorites_bookmark_is_folder_exist(const char *foldername);
int _favorites_bookmark_create_folder(const char *foldername);
int _favorites_bookmark_is_bookmark_exist(const char *url, const char *title, const int folderId);
int _favorites_get_bookmark_count_at_folder(int folderId);
int _favorites_bookmark_get_folder_count(void);
bookmark_list_h _favorites_get_bookmark_list_at_folder(int folderId);
bookmark_list_h _favorites_bookmark_get_folder_list(void);
int _favorites_get_unixtime_from_datetime(char *datetime);
int _favorites_bookmark_get_bookmark_id(const char *url, const char *title, const int folder_id);

/* history internal API */
void _favorites_history_db_close(void);
void _favorites_history_db_finalize(sqlite3_stmt *stmt);
int _favorites_history_db_open(void);
int _favorites_free_history_entry(favorites_history_entry_s *entry);

#ifdef __cplusplus
};
#endif

#endif /* __TIZEN_WEB_FAVORITES_PRIVATE_H__ */
