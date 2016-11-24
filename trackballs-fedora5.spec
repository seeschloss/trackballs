Name:           trackballs
Version:        1.1.2
Release:        1%{?dist}
Summary:        A marble game inspired by Marble Madness.
Group:          Amusements/Games
License:        GPL
URL:            http://trackballs.sourceforge.net/
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:  SDL-devel, SDL_ttf-devel, SDL_mixer-devel, guile-devel
Requires:       SDL, SDL_ttf, SDL_mixer, guile

%description
Trackballs is a marble game inspired by the 1980's Atari classic
Marble Madness.
By steering a marble ball through a labyrinth filled with sharp
objects, pools of acid and other obstacles the player collects
points.

%prep
%setup -q

%build
%configure
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc AUTHORS COPYING FAQ NEWS README README.html TODO
%{_bindir}/trackballs
%{_mandir}/man6/trackballs.6*
%{_datadir}/locale/de/LC_MESSAGES/trackballs.mo
%{_datadir}/locale/fr/LC_MESSAGES/trackballs.mo
%{_datadir}/locale/sv/LC_MESSAGES/trackballs.mo
%{_datadir}/locale/it/LC_MESSAGES/trackballs.mo
%{_datadir}/trackballs/fonts
%{_datadir}/trackballs/highScores
%{_datadir}/trackballs/images
%{_datadir}/trackballs/levels
%{_datadir}/trackballs/music
%{_datadir}/trackballs/sfx
