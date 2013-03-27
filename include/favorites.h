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

#ifndef __TIZEN_WEB_FAVORITES_H__
#define __TIZEN_WEB_FAVORITES_H__

#include <stdio.h>
#include <stdlib.h>
#include <Evas.h>
#include <tizen.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @addtogroup CAPI_WEB_FAVORITES_MODULE
 * @{
 */

/**
 * @brief Enumerations for favorites error.
 */
typedef enum favorites_error{
	FAVORITES_ERROR_NONE			= TIZEN_ERROR_NONE,              /**< Successful */
	FAVORITES_ERROR_INVALID_PARAMETER	= TIZEN_ERROR_INVALID_PARAMETER,/**< Invalid parameter */
	FAVORITES_ERROR_DB_FAILED		= TIZEN_ERROR_WEB_CLASS | 0x62,  /**< Database operation failure */
	FAVORITES_ERROR_ITEM_ALREADY_EXIST	= TIZEN_ERROR_WEB_CLASS | 0x63,  /**< Requested data already exists */
	FAVORITES_ERROR_NO_SUCH_FILE	= TIZEN_ERROR_NO_SUCH_FILE, /**< No such file or directory */
	FAVORITES_ERROR_UNKNOWN 	= TIZEN_ERROR_UNKNOWN  /**< Unknown error */
} favorites_error_e;

/**
 * @brief   The structure of bookmark entry in search results.
 *
 * @details This structure is passed to callback function in all bookmark related
 * iterations through list received from search functions.
 *
 * @see  bookmark_foreach_cb()
 */
typedef struct {
	char* address;	/**< Bookmark URL */
	char* title;	/**< The title of the bookmark */
	char* creation_date;	/**< The date of creation */
	char* update_date;	/**< The last updated date */
	char* visit_date;	/**< The last visit date */
	int id;	/**< The unique ID of bookmark */
	bool is_folder;	/**< property bookmark or folder\n @c true: folder, @c false: bookmark */
	int folder_id;	/**< The ID of parent folder */
	int order_index;	/**< The order index of bookmarks when show the list at the browser */
	bool editable;	/**< The flag of editability\n @c true : writable, @c false: read-only, not ediable */
} favorites_bookmark_entry_s;

/**
 * @brief       Called to get bookmark details for each found bookmark.
 *
 * @param[in]   item	The bookmark entry handle or folder entry handle
 * @param[in]   user_data	The user data passed from the foreach function
 *
 * @return @c true to continue with the next iteration of the loop or @c false to break out of the loop.
 *
 * @pre		favorites_bookmark_foreach() will invoke this callback.
 *
 * @see		favorites_bookmark_foreach()
 */
typedef bool (*favorites_bookmark_foreach_cb)(favorites_bookmark_entry_s *item, void *user_data);

/**
 * @brief       Gets the id of the root folder.
 *
 * @param[out]	root_folder_id	The id of the root folder.
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE        Successful
 * @retval  #FAVORITES_ERROR_DB_FAILED   Database failed
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 *
 */
int favorites_bookmark_get_root_folder_id(int *root_id);

/**
 * @brief       Adds a bookmark to bookmark database.
 *
 * @remarks  If a folder named @a "foldername" doesn't exist, it will be created in the root folder.
 * @param[in]	url	Bookmark URL
 * @param[in]	title	The title of the bookmark
 * @param[in]	folder_name	The name of parent folder in the root folder
 * @param[out]	saved_id	The unique id of the added bookmark
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE        Successful
 * @retval  #FAVORITES_ERROR_DB_FAILED   Database failed
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval  #FAVORITES_ERROR_ITEM_ALREADY_EXIST	Requested data already exists
 *
 */
int favorites_bookmark_add_bookmark(const char *url, const char *title, const char *folder_name, int *saved_id);

/**
 * @brief       Adds a bookmark or folder item  to bookmark database.
 *
 * @remarks  If the parent_id is not valid folder id, this API will return error.
 * @param[in]	title	The title of the bookmark or folder
 * @param[in]	url	The bookmark URL. if the type is folder, this param will be ignored.
 * @param[in]	parent_id	The unique id of folder which added item belong to.
 * @param[in]	type	The type of item ( 0 : bookmark, 1 : folder )
 * @param[out]	saved_id	 The unique id of the added item
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE        Successful
 * @retval  #FAVORITES_ERROR_DB_FAILED   Database failed
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval  #FAVORITES_ERROR_ITEM_ALREADY_EXIST	Requested data already exists
 * @retval  #FAVORITES_ERROR_NO_SUCH_FILE	parent_id is not valid
 *
 */
int favorites_bookmark_add(const char *title, const char *url, int parent_id, int type, int *saved_id);

/**
 * @brief       Deletes the bookmark item of given bookmark id.
 *
 * @param[in]   bookmark_id	The unique ID of bookmark to delete
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE                Successful
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval  #FAVORITES_ERROR_DB_FAILED           Database failed
 *
 */
int favorites_bookmark_delete_bookmark(int bookmark_id);

/**
 * @brief       Deletes all bookmarks and sub folders.
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE                Successful
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval  #FAVORITES_ERROR_DB_FAILED           Database failed
 *
 */
int favorites_bookmark_delete_all_bookmarks(void);

/**
 * @brief       Gets a number of bookmark list items.
 *
 * @param[out]  count   The number of bookmarks and sub folders.
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE        Successful
 * @retval  #FAVORITES_ERROR_DB_FAILED   Database failed
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 *
 */
int favorites_bookmark_get_count(int *count);

/**
 * @brief       Retrieves all bookmarks and folders by invoking the given callback function iteratively.
 *
 * @remarks  All bookmarks and folders data are also used by browser application
 * @param[in]   callback	The callback function to invoke
 * @param[in]   user_data	The user data to be passed to the callback function
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE                Successful
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval  #FAVORITES_ERROR_DB_FAILED           Database failed
 *
 * @post	This function invokes bookmark_foreach_cb() repeatedly for each bookmark.
 *
 * @see bookmark_foreach_cb()
 */
int favorites_bookmark_foreach(favorites_bookmark_foreach_cb callback, void *user_data);

/**
 * @brief       Exports a whole bookmark list as a netscape HTML bookmark file.
 *
 * @param[in]   file_path      The absolute path of the export file. This must includes html file name.
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE                Successful
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval  #FAVORITES_ERROR_DB_FAILED           Database failed
 *
 */
int favorites_bookmark_export_list(const char *file_path);

/**
 * @brief       Gets the bookmark's favicon as a evas object type
 *
 * @param[in]   bookmark_id	The unique ID of bookmark
 * @param[in]   evas	The given canvas
 * @param[out]  icon	Retrieved favicon evas object of bookmark.
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE                Successful
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval  #FAVORITES_ERROR_DB_FAILED           Database failed
 *
 */
int favorites_bookmark_get_favicon(int bookmark_id, Evas *evas, Evas_Object **icon);

/**
 * @brief       Sets the bookmark's favicon
 *
 * @param[in]   bookmark_id	The unique ID of bookmark
 * @param[in]   icon	The favicon object to save
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE                Successful
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval  #FAVORITES_ERROR_DB_FAILED           Database failed
 *
 */
int favorites_bookmark_set_favicon(int bookmark_id, Evas_Object *icon);

/**
 * @brief   The structure of history entry in search results.
 *
 * @details This structure is passed to callback function in all history related
 * iterations through list received from search functions.
 *
 * @see  history_foreach_cb()
 */
typedef struct {
	char* address;	/**< URL history */
	char* title;	/**< The title of history */
	int count;	/**< The visit count */
	char* visit_date;	/**< The last visit date */
	int id;	/**< The unique ID of history */
} favorites_history_entry_s;

/**
 * @brief       Called to get history details for each found history.
 *
 * @param[in]   item	The history entry handle
 * @param[in]   user_data	The user data passed from the foreach function
 *
 * @return @c true to continue with the next iteration of the loop or @c false to break out of the loop.
 *
 * @pre		favorites_history_foreach() will invoke this callback.
 *
 * @see		favorites_history_foreach()
 */
typedef bool (*favorites_history_foreach_cb)(favorites_history_entry_s *item, void *user_data);

/**
 * @brief       Gets a number of history list items.
 *
 * @param[out]  count   The number of histories.
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE        Successful
 * @retval  #FAVORITES_ERROR_DB_FAILED   Database failed
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 *
 */
int favorites_history_get_count(int *count);

/**
 * @brief       Retrieves all histories by invoking the given callback function iteratively.
 *
 * @param[in]   callback	The callback function to invoke
 * @param[in]   user_data	The user data to be passed to the callback function
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE                Successful
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval  #FAVORITES_ERROR_DB_FAILED           Database failed
 *
 * @post	This function invokes history_foreach_cb().
 *
 * @see history_foreach_cb()
 */
int favorites_history_foreach(favorites_history_foreach_cb callback, void *user_data);

/**
 * @brief       Deletes the history item of given history id.
 *
 * @param[in]   history_id	The history ID to delete
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE                Successful
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval  #FAVORITES_ERROR_DB_FAILED           Database failed
 *
 */
int favorites_history_delete_history(int history_id);

/**
 * @brief       Deletes the history item of given history url.
 *
 * @param[in]   url         history url which wants to be deleted
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE                Successful
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval  #FAVORITES_ERROR_DB_FAILED           Database failed
 *
 */
int favorites_history_delete_history_by_url(const char *url);

/**
 * @brief       Deletes all histories.
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE                Successful
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval  #FAVORITES_ERROR_DB_FAILED           Database failed
 *
 */
int favorites_history_delete_all_histories(void);

/**
 * @brief       Deletes all histories accessed with the browser within the specified time period.
 *
 * @param[in]   begin_date         The start date of the period
 * @param[in]   end_date           The end date of the period
 *
 * @remarks  Date format must be "yyyy-mm-dd hh:mm:ss" ex: "2000-01-01 01:20:35".
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE                Successful
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval  #FAVORITES_ERROR_DB_FAILED           Database failed
 *
 */
int favorites_history_delete_history_by_term(const char *begin_date, const char *end_date);

/**
 * @brief       Gets the history's favicon as an evas object type
 *
 * @param[in]   history_id	The unique ID of history item
 * @param[in]   evas	The given canvas
 * @param[out]  icon	Retrieved favicon evas object of bookmark.
 *
 * @return  0 on success, otherwise a negative error value.
 * @retval  #FAVORITES_ERROR_NONE                Successful
 * @retval  #FAVORITES_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval  #FAVORITES_ERROR_DB_FAILED           Database failed
 *
 */
int favorites_history_get_favicon(int history_id, Evas *evas, Evas_Object **icon);

/**
 * @}
 */

#ifdef __cplusplus
};
#endif

#endif /* __TIZEN_WEB_FAVORITES_H__ */
