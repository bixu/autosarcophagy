#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

int attempt() {
    int errorCode = 0;

    // Mangle the file and save it to test.c
    if(0 != (errorCode = mangle("autosarcophagy.c", "test.c"))) {
        fprintf(stderr, "mangle failed\n");
        return errorCode;
    }

    // Attempt to Compile It
    if(0 != (errorCode = compile("test.c", "test"))) {
        return errorCode;
    }

    return 0;
}

int mangle(const char* from, const char *to) {
    int fdFrom;
    int fdTo;
    char buf[4096];
    ssize_t nRead;
    int err;

    if(0 > (fdFrom = open(from, O_RDONLY))) {
        return -1;
    }

    if(0 > (fdTo = open(to, O_WRONLY | O_CREAT, 0666))) {
        return -1;
    }

    while(0 < (nRead = read(fdFrom, buf, sizeof buf))) {

        // make one change
        int offset = rand() % (sizeof buf);
        buf[offset] = ' ' + ((rand() % '~') - ' ');

        write(fdTo, &buf, nRead);
    }

    fsync(fdTo);

    if(0 > close(fdTo)) {
        return -1;
    }

    if(0 > close(fdFrom)) {
        return -1;
    }

    sync();

    return 0;
}

int compile(const char *source, const char *binary) {
    int status = -1;

    pid_t pid = fork();

    if(0 == pid) {
        int fd = open("/dev/null", O_WRONLY);
        dup2(fd, 1);
        dup2(fd, 2);
        sync();
        if(0 > execl("/usr/bin/gcc", "gcc", source, "-o", binary, (char *)0)) {
            perror("exec for gcc failed");
        }

    }
    else if(0 > pid) {
        perror("compile fork failed");
        return -1;
    }
    else {
        if(0 > waitpid(pid, &status, 0)) {
            perror("waiting for child failed");
        }
    }

    return status;
}

int commit() {
    int status = 0;

    pid_t pid = fork();

    return 0;

    if(0 == pid) {
        fprintf(stderr, "committing\n");
        execl("/usr/bin/git", "git", "commit", "-a", "-m", "this compiles", (char *)0);
    }
    else {
        waitpid(pid, &status, 0);
    }

    if(0 != status) {
        return status;
    }

    return 0;
}

int push() {
    int status = 0;

    pid_t pid = fork();

    if(0 == pid) {
        fprintf(stderr, "pushing\n");
        execl("/usr/bin/git", "git", "push", (char *)0);
    }
    else {
        waitpid(pid, &status, WNOHANG);
    }

    return status;
}

int copy(const char* from, const char *to) {
    int fdFrom;
    int fdTo;
    char buf[4096];
    ssize_t nRead;
    int err;

    if(0 > (fdFrom = open(from, O_RDONLY))) {
        return -1;
    }

    if(0 > (fdTo = open(to, O_WRONLY | O_CREAT, 0666))) {
        return -1;
    }

    while(0 < (nRead = read(fdFrom, buf, sizeof buf))) {
        write(fdTo, &buf, nRead);
    }

    fsync(fdTo);

    if(0 > close(fdTo)) {
        return -1;
    }

    if(0 > close(fdFrom)) {
        return -1;
    }


    return 0;
}

int switchBinary(const char *binary) {
    sync();
    return execl(binary, binary, (char *)0);
}

int main(int argc, char **argv) {
    int errorCode = 1;

    srand(time(NULL));

    // Keep running until something compiles and is workable
    while(0 != errorCode ) {
        errorCode = attempt();
        if(!(rand() % 800)) {
            fprintf(stderr, ".");
        }
    }

    fprintf(stderr, "+");

    // Copy the file
    if(0 > (errorCode = copy("test.c", "autosarcophagy.c"))) {
        fprintf(stderr, "Failed to copy test.c to autosarcophagy.c\n");
    }

    // Compile It
    if(0 != (errorCode = compile("autosarcophagy.c", "autosarcophagy"))) {
        perror("Error while compiling autosarcophagy");
    }
    else {
        // Commit the change
        if(0 != (errorCode = commit())) {
            fprintf(stderr, "Failed to commit changes\n");
        }
    }

    // Switch to it
    if(0 != switchBinary("./autosarcophagy")) {
        perror("Error trying to swap binaries");
    }

    return 0;
}
