Name:       capi-web-favorites
Summary:    Internet bookmark and history API
Version:    0.0.7
Release:    1
Group:      TO_BE/FILLED_IN
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(capi-base-common)
Requires(post): /sbin/ldconfig  
Requires(postun): /sbin/ldconfig

%description


%package devel
Summary:  Internet Bookmark and History in Tizen Native API (Development)
Group:    TO_BE/FILLED_IN
Requires: %{name} = %{version}-%{release}

%description devel



%prep
%setup -q


%build
cmake . -DCMAKE_INSTALL_PREFIX=/usr


make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%post
/sbin/ldconfig
mkdir -p /opt/usr/dbspace/
##### History ######
if [ ! -f /opt/usr/dbspace/.browser-history.db ];
then
	sqlite3 /opt/usr/dbspace/.browser-history.db 'PRAGMA journal_mode=PERSIST;
	CREATE TABLE history(id INTEGER PRIMARY KEY AUTOINCREMENT, address, title, counter INTEGER, visitdate DATETIME, favicon BLOB, favicon_length INTEGER, favicon_w INTEGER, favicon_h INTEGER);'
fi

### Bookmark ###
if [ ! -f /opt/usr/dbspace/.internet_bookmark.db ];
then
	sqlite3 /opt/usr/dbspace/.internet_bookmark.db 'PRAGMA journal_mode=PERSIST;
	CREATE TABLE bookmarks(id INTEGER PRIMARY KEY AUTOINCREMENT, type INTEGER, parent INTEGER, address, title, creationdate, sequence INTEGER, updatedate, visitdate, editable INTEGER, accesscount INTEGER, favicon BLOB, favicon_length INTEGER, favicon_w INTEGER, favicon_h INTEGER);
	create index idx_bookmarks_on_parent_type on bookmarks(parent, type);

	insert into bookmarks (type, parent, title, creationdate, editable, sequence, accesscount) values(1, 0, "Bookmarks", DATETIME("now"),  0, 1, 0);'
fi

# Change db file owner & permission
chown :5000 /opt/usr/dbspace/.browser-history.db
chown :5000 /opt/usr/dbspace/.browser-history.db-journal
chown :5000 /opt/usr/dbspace/.internet_bookmark.db
chown :5000 /opt/usr/dbspace/.internet_bookmark.db-journal
chmod 666 /opt/usr/dbspace/.browser-history.db
chmod 666 /opt/usr/dbspace/.browser-history.db-journal
chmod 666 /opt/usr/dbspace/.internet_bookmark.db
chmod 666 /opt/usr/dbspace/.internet_bookmark.db-journal

if [ -f /usr/lib/rpm-plugins/msm.so ]
then
	chsmack -a 'org.tizen.browser::db_external' /opt/usr/dbspace/.browser-history.db*
	chsmack -a 'org.tizen.browser::db_external' /opt/usr/dbspace/.internet_bookmark.db*
fi
##################################################
# set default vconf values
##################################################
#internal keys
vconftool set -t string db/browser/browser_user_agent "System user agent" -g 5000 -f
vconftool set -t string db/browser/custom_user_agent "" -g 5000 -f
#public keys
vconftool set -t string db/browser/user_agent "Mozilla/5.0 (Linux; U; Tizen 2.0; en-us) AppleWebKit/537.1 (KHTML, like Gecko) Version/2.0 Mobile" -g 5000 -f

%postun -p /sbin/ldconfig


%files
%manifest capi-web-favorites.manifest
%{_libdir}/libcapi-web-favorites.so

%files devel
%{_includedir}/web/*.h
%{_libdir}/pkgconfig/*.pc


