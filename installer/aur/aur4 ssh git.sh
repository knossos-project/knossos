#!/bin/bash
#just let something like smartgit handle the ssh authentication
#ssh-agent bash -c 'ssh-add id_rsa; git clone ssh://aur@aur4.archlinux.org/knossos.git'
ssh-agent bash -c 'ssh-add ../id_rsa; git push'
