: "${CR_PAT:?CR_PAT is not set; you might also want to adjust the user in this script}"
docker commit $(docker container list -q) ghcr.io/knossos-project/arch:multi
docker export $(docker container list -q) | docker import - ghcr.io/knossos-project/arch
echo $CR_PAT | docker login ghcr.io -u Optiligence --password-stdin
docker push ghcr.io/knossos-project/arch:multi
docker push ghcr.io/knossos-project/arch
