FROM archlinux:latest AS base

# Install enought to run makepkg 
# PKGBUILD from the repo will pull all dependencies for build and install
RUN pacman -Suy --noconfirm base-devel git

ARG AUR_PACKAGE=trunk-recorder-git

RUN useradd builduser -m \
  && passwd -d builduser \
  && cd /home/builduser \
  && git clone "https://aur.archlinux.org/$AUR_PACKAGE.git" target \
  && chown builduser -R target \
  && (printf 'builduser ALL=(ALL) ALL\n' | tee -a /etc/sudoers) \
  && sudo -u builduser bash -c 'cd ~/target && makepkg -si --noconfirm' \
  && pacman -Rns --noconfirm $(pacman -Qtdq)

# GNURadio requires a place to store some files, can only be set via $HOME env var.
ENV HOME=/tmp

CMD ["/usr/bin/trunk-recorder", "--config=/app/config.json"]