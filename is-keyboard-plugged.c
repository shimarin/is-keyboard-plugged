#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>

static int read_sysfs_attr(const char *path, char *buf, size_t bufsize)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int ok = (fgets(buf, bufsize, f) != NULL);
    fclose(f);
    if (!ok) return -1;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    return 0;
}

int main(int argc, char *argv[])
{
    int verbose = 0, inverse = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "--inverse") == 0) {
            inverse = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [--verbose] [--inverse] [--help]\n"
                   "Exits 0 if a USB keyboard (HID class, boot protocol) is connected.\n"
                   "  --inverse  invert exit code (0 = no keyboard, 1 = keyboard found)\n"
                   "  --verbose  print matched device info, or 'No USB keyboards.' if none\n",
                   argv[0]);
            return 0;
        }
    }

    glob_t gl;
    if (glob("/sys/bus/usb/devices/*/bInterfaceClass", 0, NULL, &gl) != 0)
        return 1;

    int found = 0;
    char buf[256], path[4096], dir[2048];

    for (size_t i = 0; i < gl.gl_pathc; i++) {
        /* dir = /sys/bus/usb/devices/X-Y:A.B */
        strncpy(dir, gl.gl_pathv[i], sizeof(dir) - 1);
        char *slash = strrchr(dir, '/');
        if (!slash) continue;
        *slash = '\0';

        if (read_sysfs_attr(gl.gl_pathv[i], buf, sizeof(buf)) < 0) continue;
        if (strcmp(buf, "03") != 0) continue;

        snprintf(path, sizeof(path), "%s/bInterfaceProtocol", dir);
        if (read_sysfs_attr(path, buf, sizeof(buf)) < 0) continue;
        if (strcmp(buf, "01") != 0) continue;

        found = 1;

        if (verbose) {
            /* Parent device dir: strip ":A.B" suffix from the interface name */
            char *iface_name = strrchr(dir, '/');
            char parent_id[64] = "";
            if (iface_name) {
                strncpy(parent_id, iface_name + 1, sizeof(parent_id) - 1);
                char *colon = strchr(parent_id, ':');
                if (colon) *colon = '\0';
            }

            char parent_path[2048];
            snprintf(parent_path, sizeof(parent_path),
                     "/sys/bus/usb/devices/%s", parent_id);

            char manufacturer[256] = "";
            char product[256] = "(unknown)";
            char idvendor[16] = "????";
            char idproduct[16] = "????";

            snprintf(path, sizeof(path), "%s/manufacturer", parent_path);
            read_sysfs_attr(path, manufacturer, sizeof(manufacturer));
            snprintf(path, sizeof(path), "%s/product", parent_path);
            read_sysfs_attr(path, product, sizeof(product));
            snprintf(path, sizeof(path), "%s/idVendor", parent_path);
            read_sysfs_attr(path, idvendor, sizeof(idvendor));
            snprintf(path, sizeof(path), "%s/idProduct", parent_path);
            read_sysfs_attr(path, idproduct, sizeof(idproduct));

            printf("%s %s [%s:%s] (interface: %s)\n",
                   manufacturer, product, idvendor, idproduct, dir);
        }
    }

    globfree(&gl);
    if (!found && verbose) puts("No USB keyboards.");
    return (found ^ inverse) ? 0 : 1;
}
