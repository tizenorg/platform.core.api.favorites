Name:       capi-web-favorites
Summary:    Internet bookmark and history API
Version:    0.0.15
Release:    1
Group:      Web Framework/API
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: 	capi-web-favorites.manifest
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(libtzplatform-config)
Requires(post): /sbin/ldconfig  
Requires(postun): /sbin/ldconfig

%description
API for Internet Bookmarks and History

%package devel
Summary:  Internet Bookmark and History in Tizen Native API (Development)
Group:    Web Framework/Development
Requires: %{name} = %{version}-%{release}

%description devel
Development package for favorites API. Favorites API provides teh framework for bookmarks and history

%prep
%setup -q
cp %{SOURCE1001} .


%build
%cmake . -DTZ_SYS_SHARE=%TZ_SYS_SHARE
make %{?jobs:-j%jobs}

%install
%make_install

%post
/sbin/ldconfig

users_gid=$(getent group $TZ_SYS_USER_GROUP | cut -f3 -d':')

# set default vconf values
##################################################
#internal keys
vconftool set -t string db/browser/browser_user_agent "System user agent" -g $users_gid -f
vconftool set -t string db/browser/custom_user_agent "" -g $users_gid -f
#public keys
vconftool set -t string db/browser/user_agent "Mozilla/5.0 (Linux; Tizen 2.1; sdk) AppleWebKit/537.3 (KHTML, like Gecko) Version/2.1 Mobile Safari/537.3" -g $users_gid -f

%postun -p /sbin/ldconfig


%files
%manifest %{name}.manifest
%{_libdir}/libcapi-web-favorites.so
%attr(0755,root,%TZ_SYS_USER_GROUP) %TZ_SYS_SHARE/%{name}/internet_bookmark_DB_init.sh
%attr(0755,root,%TZ_SYS_USER_GROUP) %TZ_SYS_SHARE/%{name}/browser_history_DB_init.sh

%files devel
%manifest %{name}.manifest
%{_includedir}/web/*.h
%{_libdir}/pkgconfig/*.pc
