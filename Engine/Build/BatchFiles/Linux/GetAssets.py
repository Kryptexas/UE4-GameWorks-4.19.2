#!/usr/bin/env python
# returns 
#  0 if assets are up to date
#  1 if there was any kind of error 
#  2 if assets were not up to date, but have been successfully downloaded
import os
import json
try:
	from urllib2 import urlopen, Request
except:
	from urllib.request import urlopen, Request
from sys import argv, stdout
import time

argv0 = os.path.basename(argv[0])

if len(argv) < 3:
	print("Usage: {0} <username/repo> <release-tag>".format(argv0))
	print("")
	print("example: {0}  EpicGames/UnrealEngine latest-preview".format(argv0))
	exit(1)

try:
	token=os.environ["OAUTH_TOKEN"]
except:
	print("Please set OAUTH_TOKEN")
	print("You can generate an access token at https://github.com/settings/tokens/new")
	print("It must have repo scope")
	exit(1)

try:
	download_path=os.environ["ARCHIVE_ROOT"]
except:
	download_path=os.path.expanduser("~/Downloads")



repo = argv[1]
release_name = argv[2]

#only_names = set(argv[3:]) #later...

def get_releases(repo):
	try:
		return json.loads(urlopen("https://api.github.com/repos/{0}/releases?access_token={1}".format(repo, token)).read().decode("utf-8"))
	except:
		return None

def progress(done, sofar):
	filled = int((sofar * 72) / done)
	return "{0} {1} {2:4.1f}%".format("#" * filled, " " * (72-filled),  (float(sofar)/float(done)*100))

def download_assets(repo):
	releases = get_releases(repo)
	if not releases:
		if not repo == "EpicGames/UnrealEngine":
			return download_assets("EpicGames/UnrealEngine")
		else:
			print("Unable to download assets for release {0}!".format(release_name))
			exit(1)

	had_to_download=False
	for release in releases:
		if not release["tag_name"] == release_name:
			continue
	
		if len(release["assets"]) == 0:
			break
		for asset in release["assets"]:
			name = asset["name"]
			asset_path = os.path.join(download_path, release_name+"-"+name)
			at_latest_version=False
	
			try:
				with open(asset_path+".updated") as f:
					timefmt = "%Y-%m-%dT%H:%M:%SZ"
					downloaded = time.strptime(f.read(), timefmt)
					updated = time.strptime(asset['updated_at'], timefmt)
					at_latest_version = downloaded >= updated
			except:
				pass
	
			size = int(asset["size"])
			if at_latest_version:
				if size != os.path.getsize(asset_path):
					print("Incomplete download of asset {0}, retrying")
				else:
					print("Asset {0} up to date".format(name))
					continue
	
			had_to_download=True

			req = Request(asset["url"]+"?access_token="+token, None, {"Accept": "application/octet-stream"})
			handle = urlopen(req)
			length = int(handle.headers["Content-Length"])
	
			print("Started downloading {0}".format(name))
	
			with open(asset_path, "wb") as f:
				read = 0
				while True:
					chunk = handle.read(4096)
					if len(chunk):
						f.write(chunk)
					else:
						break
					read += len(chunk)
					stdout.write("\r" + progress(length, read))
					stdout.flush()
			with open(asset_path+".updated", 'w') as f:
				f.write(asset['updated_at'])
			print("\nFinished downloading {0}".format(name))
	
		if had_to_download:
			return 2
		return 0
	if not repo == "EpicGames/UnrealEngine":
		return download_assets("EpicGames/UnrealEngine")

	print("Unable to download assets for release {0}!".format(release_name))
	exit(1)

exit(download_assets(repo))

