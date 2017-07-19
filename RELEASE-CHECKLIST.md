1. Open gitk and review changes since last release.
   * Document them in NEWS,  All API and API semantic changes should be clearly
     marked as API additions, API changes, or API deletions.
     If there's a backward-incompatible API change (including deletions for API used anywhere),
     that's a release blocker.  Do NOT release.  Document deprecations.
2. Based on severity of changes, decide whether it's a minor or micro release number bump,
3. Make sure you have correct date and new version at the top of NEWS file,
4. Bump version in configure.ac line 3,
5. Do "make distcheck", if it passes, you get a tarball.
   Otherwise, fix things and commit them separately before making release,
6. "make release-files". Enter your GPG password again.  This creates a sha256 hash and signs it.
7. Now that you have a tarball built, commit NEWS and configure.ac changes.  The commit message
   is simply the release number.  Eg. "1.4.7"
8. Tag the release and sign it: Eg. "git tag -s 1.4.7 -m 1.4.7".  Enter your GPG password.
9. Build win32 bundle (WIP).
10. Copy all artefacts to users.freedesktop.org and move them into `/srv/www.freedesktop.org/www/software/harfbuzz/release`
    There should be four files.  Eg.:
 ```
-rw-r--r--  1 behdad eng 1592693 Jul 18 11:25 harfbuzz-1.4.7.tar.bz2
-rw-r--r--  1 behdad eng      89 Jul 18 11:34 harfbuzz-1.4.7.tar.bz2.sha256
-rw-r--r--  1 behdad eng     339 Jul 18 11:34 harfbuzz-1.4.7.tar.bz2.sha256.asc
-rw-r--r--  1 behdad eng 2895619 Jul 18 11:34 harfbuzz-1.4.7-win32.zip
```
11. While doing that, quickly double-check the size of the .tar.bz2 and .zip files against their previous releases to make sure nothing bad happened.  They should be in the ballpark, perhaps slightly larger.  Sometimes they do shrink, that's not by itself a stopper.
12. Push the commit and tag out: "git push --follow-tags".  Make sure it's pushed both to freedesktop repo and github.
13. Upload artefacts and NEWS entry on the github release.
