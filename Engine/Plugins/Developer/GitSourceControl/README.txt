Unreal Engine 4 Git Source Control Plugin
-----------------------------------------

UE4GitPlugin is a simple Git Source Control Plugin for Unreal Engine 4.8

Copyright (c) 2014-2015 Sebastien Rombauts (sebastien.rombauts@gmail.com)

### Beta version 0.6.3:
- display status icons to show modified/added/deleted/untracked files
- show history of a file
- diff against depot or between previous versions of a file
- revert modifications of a file
- add a file
- delete a file
- checkin/commit a file (cannot handle atomically more than 20 files)
- show current branch name in status text
- solve a merge conflict

### What *cannot* be done presently (TODO list for v1.0, ordered by priority):
- merge blueprints
- initialize a new Git local repository ('git init') to manage your UE4 Game Project.
- tags: implement ISourceControlLabel to manage git tags
- .uproject file state si not visible in the current Editor
- Branch is not in the current Editor workflow (but on Epic Roadmap)
- Pull/Fetch/Push are not in the current Editor workflow
- Amend a commit is not in the current Editor workflow
- configure user name & email ('git config user.name' & git config user.email')

### Known issues:
- the Editor does not show deleted files (only when deleted externaly?)
- the Editor does not show missing files
- missing localisation for git specific messages
- migrate an asset should add it to the destination project if also under Git (needs management of 'out of tree' files)
- displaying states of 'Engine' assets (also needs management of 'out of tree' files)
- issue #22: A Move/Rename leaves a redirector file behind
- issue #11: Add the "Resolve" operation introduced in Editor 4.3
- improve the 'Init' window text, hide it if connection is already done, auto connect
- reverting an asset does not seem to update content in Editor! Issue in Editor?
- file history show Revision as signed integer instead of hexadecimal SHA1
- file history does not report file size
- standard Editor commit dialog ask if user wants to "Keep Files Checked Out" => no use for Git or Mercurial CanCheckOut()==false
- Windows only (64 bits) -> Mac compiles but needs testing/debugging (Linux source control is not supported by Editor)

### Wishlist (after v1.0):
- [git-annexe and/or git-media - #1 feature request](https://github.com/SRombauts/UE4GitPlugin/issues/1)

### In-code TODO list (internal roadmap):

- FGitConnectWorker::Execute (While project not in Git source control)
  Improve error message "You should check out a working copy..."
  => double error message (and in reverse order) with "Project is not part of a Git working copy"

- FGitResolveWorker (GitSourceControlOperations.h)
  git add to mark a conflict as resolved
  
- FGitSourceControlRevision::GetBranchSource() const
  if this revision was copied from some other revision, then that source revision should
	be returned here (this should be determined when history is being fetched)
- FGitSourceControlState::GetBaseRevForMerge()
  get revision of the merge-base (https://www.kernel.org/pub/software/scm/git/docs/git-merge-base.html)
	
- FGitConnectWorker::Execute()
  popup to propose to initialize the git repository "git init + .gitignore"

- FGitSyncWorker (GitSourceControlOperations.h)
  git fetch remote(s) to be able to show files not up-to-date with the serveur
- FGitSourceControlState::IsCurrent() const
  check the state of the HEAD versus the state of tracked branch on remote

- FGitSourceControlRevision::GetFileSize() const
	git log does not give us the file size, but we could run a specific command

- GitSourceControlUtils::CheckGitAvailability
  also check Git config user.name & user.email

Windows:
- GitSourceControlUtils::FindGitBinaryPath
  use the Windows registry to find Git

Mac:
- GitSourceControlUtils::RunCommandInternalRaw
  Specifying the working copy (the root) of the git repository (before the command itself)
	does not work in UE4.1 on Mac if there is a space in the path ("/Users/xxx/Unreal Project/MyProject")

Bug reports?
- FGitSourceControlRevision::Get
  Bug report: a counter is needed to avoid overlapping files; temp files are not (never?) released by Editor!

- GitSourceControlUtils::UpdateCachedStates
  // State->TimeStamp = InState.TimeStamp; // Bug report: Workaround a bug with the Source Control Module not updating file state after a "Save"
