%global version {{version}}

Name:		oneclient
Version:	%{version}
Release:	{{build}}%{?dist}
Summary:	FUSE client for onedata system
Group:		Applications/File
License:	MIT
URL:		https://onedata.org
Source0:	oneclient-%{version}.orig.tar.gz

BuildRequires: aws-sdk-cpp
BuildRequires: boost-devel >= 1.58.0,
BuildRequires: cmake >= 3.0.0,
BuildRequires: fuse-devel >= 2.7,
BuildRequires: gcc-c++ >= 5.0.0,
BuildRequires: git,
BuildRequires: glog-devel >= 0.3.4,
BuildRequires: golang,
BuildRequires: libcurl-devel,
BuildRequires: librados2-devel,
BuildRequires: libsodium-devel,
BuildRequires: libtool-ltdl,
BuildRequires: libtool-ltdl-devel,
BuildRequires: openssl,
BuildRequires: poco-devel,
BuildRequires: poco-foundation,
BuildRequires: poco-netssl,
BuildRequires: poco-util,
BuildRequires: poco-xml,
BuildRequires: openssl-devel,
BuildRequires: protobuf-compiler >= 2.6.0,
BuildRequires: protobuf-devel >= 2.6.0,
BuildRequires: python-devel,
BuildRequires: subversion,
BuildRequires: swift-sdk-cpp,
BuildRequires: tbb-devel >= 4.3~,

%description
oneclient is a software based on FUSE (Filesystem in Userspace) that allows mounting onedata filesystem on Linux systems.

%prep
%setup -q


%build
%cmake . -DBUILD_INTEGRATION_TESTS=Off -DBUILD_SHARED_LIBS=Off
make %{?_smp_mflags} oneclient


%install
make install DESTDIR=%{buildroot}
find %{buildroot}


%files
%{_bindir}/oneclient
%{_sysconfdir}/oneclient.conf

%doc



%changelog
