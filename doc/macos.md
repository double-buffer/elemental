xattr -l /path/to/application.app
You'll probably find com.apple.quarantine listed. If so, you can get rid of it with

xattr -dr com.apple.quarantine /path/to/application.app
