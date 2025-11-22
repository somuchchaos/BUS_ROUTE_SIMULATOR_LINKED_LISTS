/* bus_route.c
   Bus Route Simulator using circular doubly-linked list
   Features:
   - view full route
   - search any stop
   - insert stops (end / after stop / at position)
   - delete stop
   - see passengers waiting at a stop
   - total time & distance of route
   - distance/time between any two stops
   - save/load route to CSV
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_LEN 64
#define LINE_LEN 256

typedef struct Stop {
    int id;
    char name[NAME_LEN];
    int passengers;            // waiting passengers
    double dist_to_next;       // kilometers to next stop
    double time_to_next;       // minutes to next stop
    struct Stop *prev;
    struct Stop *next;
} Stop;

Stop *head = NULL;
int next_id = 1;

/* Utility - create a new stop node */
Stop* create_stop(const char *name, int passengers, double dist_to_next, double time_to_next) {
    Stop *s = (Stop*)malloc(sizeof(Stop));
    if (!s) { perror("malloc"); exit(EXIT_FAILURE); }
    s->id = next_id++;
    strncpy(s->name, name, NAME_LEN-1);
    s->name[NAME_LEN-1] = '\0';
    s->passengers = passengers;
    s->dist_to_next = dist_to_next;
    s->time_to_next = time_to_next;
    s->prev = s->next = NULL;
    return s;
}

/* Insert at end (if empty, becomes head) */
void insert_end(Stop *node) {
    if (!node) return;
    if (!head) {
        head = node;
        head->next = head->prev = head;
    } else {
        Stop *tail = head->prev;
        tail->next = node;
        node->prev = tail;
        node->next = head;
        head->prev = node;
    }
}

/* View full route (start from head) */
void view_route() {
    if (!head) { printf("Route is empty.\n"); return; }
    printf("Full route:\n");
    Stop *cur = head;
    int idx = 1;
    do {
        printf("%2d) ID:%d  Name:\"%s\"  Passengers:%d  dist_to_next:%.2f km  time_to_next:%.2f min\n",
               idx, cur->id, cur->name, cur->passengers, cur->dist_to_next, cur->time_to_next);
        cur = cur->next; idx++;
    } while (cur != head);
}

/* Find stop by name (first match, case-insensitive) */
Stop* find_by_name(const char *name) {
    if (!head) return NULL;
    Stop *cur = head;
    do {
        if (strcasecmp(cur->name, name) == 0) return cur;
        cur = cur->next;
    } while (cur != head);
    return NULL;
}

/* Find by id */
Stop* find_by_id(int id) {
    if (!head) return NULL;
    Stop *cur = head;
    do {
        if (cur->id == id) return cur;
        cur = cur->next;
    } while (cur != head);
    return NULL;
}

/* Insert a new stop after a given existing stop pointer */
void insert_after(Stop *existing, Stop *newstop) {
    if (!newstop) return;
    if (!existing) { // insert as first node / end
        insert_end(newstop);
        return;
    }
    Stop *nxt = existing->next;
    existing->next = newstop;
    newstop->prev = existing;
    newstop->next = nxt;
    nxt->prev = newstop;
}

/* Insert at position (1-based). If pos > length+1, insert at end */
void insert_at_position(Stop *newstop, int pos) {
    if (!newstop) return;
    if (!head || pos <= 1) {
        if (!head) {
            head = newstop;
            head->next = head->prev = head;
        } else {
            Stop *tail = head->prev;
            newstop->next = head;
            newstop->prev = tail;
            tail->next = newstop;
            head->prev = newstop;
            head = newstop;
        }
        return;
    }
    Stop *cur = head;
    int idx = 1;
    while (cur->next != head && idx < pos-1) {
        cur = cur->next; idx++;
    }
    insert_after(cur, newstop);
}

/* Delete stop by name (first match) */
int delete_by_name(const char *name) {
    Stop *target = find_by_name(name);
    if (!target) return 0;
    if (target->next == target) { // only node
        free(target);
        head = NULL;
        return 1;
    }
    Stop *p = target->prev;
    Stop *n = target->next;
    p->next = n;
    n->prev = p;
    if (target == head) head = n;
    free(target);
    return 1;
}

/* Total distance and time for full route */
void total_distance_time(double *tot_dist, double *tot_time) {
    *tot_dist = *tot_time = 0.0;
    if (!head) return;
    Stop *cur = head;
    do {
        *tot_dist += cur->dist_to_next;
        *tot_time += cur->time_to_next;
        cur = cur->next;
    } while (cur != head);
}

/* Distance/time between two stops by name.
   Walk forward from start to target and accumulate; if not found returns -1.
   If start==target, distance/time = 0. */
int distance_between(const char *a_name, const char *b_name, double *dist_out, double *time_out) {
    *dist_out = *time_out = 0.0;
    if (!head) return 0;
    Stop *start = find_by_name(a_name);
    Stop *target = find_by_name(b_name);
    if (!start || !target) return 0; // not found
    if (start == target) return 1; // zero distance/time
    Stop *cur = start;
    do {
        *dist_out += cur->dist_to_next;
        *time_out += cur->time_to_next;
        cur = cur->next;
        if (cur == target) return 1;
    } while (cur != start);
    // If we complete full loop and didn't hit target, it means target wasn't in list (shouldn't happen)
    return 0;
}

/* Save route to CSV: id,name,passengers,dist_to_next,time_to_next */
int save_to_file(const char *filename) {
    if (!head) { printf("No route to save.\n"); return 0; }
    FILE *f = fopen(filename, "w");
    if (!f) { perror("fopen"); return 0; }
    fprintf(f, "id,name,passengers,dist_to_next,time_to_next\n");
    Stop *cur = head;
    do {
        fprintf(f, "%d,%s,%d,%.6f,%.6f\n", cur->id, cur->name, cur->passengers, cur->dist_to_next, cur->time_to_next);
        cur = cur->next;
    } while (cur != head);
    fclose(f);
    return 1;
}

/* Clear current list freeing memory */
void clear_route() {
    if (!head) return;
    Stop *cur = head->next;
    while (cur != head) {
        Stop *tmp = cur;
        cur = cur->next;
        free(tmp);
    }
    free(head);
    head = NULL;
}

/* Load route from CSV. File format: id,name,passengers,dist_to_next,time_to_next
   This will clear the existing route. IDs in file are ignored and reassigned.
*/
int load_from_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { perror("fopen"); return 0; }
    clear_route();
    char line[LINE_LEN];
    // skip header
    if (!fgets(line, LINE_LEN, f)) { fclose(f); return 0; }
    while (fgets(line, LINE_LEN, f)) {
        char name[NAME_LEN];
        int passengers;
        double dist, time;
        // parse CSV line - simple parsing assuming no commas in name
        if (sscanf(line, "%*d,%63[^,],%d,%lf,%lf", name, &passengers, &dist, &time) >= 4) {
            Stop *s = create_stop(name, passengers, dist, time);
            insert_end(s);
        }
    }
    fclose(f);
    return 1;
}

/* Display a stop info */
void print_stop(Stop *s) {
    if (!s) return;
    printf("ID:%d  Name:\"%s\"  Passengers:%d  dist_to_next:%.2f km  time_to_next:%.2f min\n",
           s->id, s->name, s->passengers, s->dist_to_next, s->time_to_next);
}

/* Small helpers for CLI input */
void read_line(char *buf, int n) {
    if (fgets(buf, n, stdin)) {
        size_t L = strlen(buf);
        if (L && buf[L-1] == '\n') buf[L-1] = '\0';
    } else {
        buf[0] = '\0';
    }
}

double read_double(const char *prompt) {
    char tmp[64];
    while (1) {
        printf("%s", prompt);
        read_line(tmp, sizeof(tmp));
        if (strlen(tmp) == 0) return 0.0;
        char *end;
        double v = strtod(tmp, &end);
        if (end != tmp) return v;
        printf("Invalid number, try again.\n");
    }
}

int read_int(const char *prompt) {
    char tmp[64];
    while (1) {
        printf("%s", prompt);
        read_line(tmp, sizeof(tmp));
        if (strlen(tmp) == 0) return 0;
        char *end;
        long v = strtol(tmp, &end, 10);
        if (end != tmp) return (int)v;
        printf("Invalid integer, try again.\n");
    }
}

/* Populate with sample data useful for demo/testing */
void populate_sample() {
    clear_route();
    next_id = 1;
    insert_end(create_stop("Central Station", 12, 2.5, 6.0));
    insert_end(create_stop("Market Road", 5, 1.2, 3.0));
    insert_end(create_stop("Library", 3, 0.9, 2.0));
    insert_end(create_stop("College", 8, 1.8, 4.0));
    insert_end(create_stop("Park", 2, 2.0, 5.0));
    printf("Sample route populated.\n");
}

/* CLI main loop */
void menu() {
    char choice[8];
    char buf[LINE_LEN];
    while (1) {
        printf("\n--- Bus Route Simulator ---\n");
        printf("1) View full route\n");
        printf("2) Search stop by name\n");
        printf("3) Insert stop (end)\n");
        printf("4) Insert stop (after a stop)\n");
        printf("5) Insert stop (position)\n");
        printf("6) Delete stop by name\n");
        printf("7) Passengers waiting at a stop\n");
        printf("8) Total distance & time\n");
        printf("9) Distance & time between two stops\n");
        printf("10) Save route to CSV\n");
        printf("11) Load route from CSV\n");
        printf("12) Populate sample route (demo)\n");
        printf("0) Exit\n");
        printf("Choose option: ");
        read_line(choice, sizeof(choice));
        if (strcmp(choice, "1") == 0) {
            view_route();
        } else if (strcmp(choice, "2") == 0) {
            printf("Enter stop name: ");
            read_line(buf, sizeof(buf));
            Stop *s = find_by_name(buf);
            if (s) print_stop(s);
            else printf("Stop not found.\n");
        } else if (strcmp(choice, "3") == 0) {
            char name[NAME_LEN];
            printf("Enter new stop name: ");
            read_line(name, sizeof(name));
            int p = read_int("Enter waiting passengers (int): ");
            double d = read_double("Enter distance to next stop (km): ");
            double t = read_double("Enter time to next stop (min): ");
            insert_end(create_stop(name, p, d, t));
            printf("Inserted at end.\n");
        } else if (strcmp(choice, "4") == 0) {
            char name[NAME_LEN], after[NAME_LEN];
            printf("Enter new stop name: ");
            read_line(name, sizeof(name));
            printf("Insert after which stop (name)? ");
            read_line(after, sizeof(after));
            int p = read_int("Enter waiting passengers (int): ");
            double d = read_double("Enter distance to next stop (km): ");
            double t = read_double("Enter time to next stop (min): ");
            Stop *s = create_stop(name, p, d, t);
            Stop *existing = find_by_name(after);
            if (!existing) {
                insert_end(s);
                printf("After-stop not found; appended at end.\n");
            } else {
                insert_after(existing, s);
                printf("Inserted after \"%s\"\n", existing->name);
            }
        } else if (strcmp(choice, "5") == 0) {
            char name[NAME_LEN];
            printf("Enter new stop name: ");
            read_line(name, sizeof(name));
            int pos = read_int("Enter position (1-based): ");
            int p = read_int("Enter waiting passengers (int): ");
            double d = read_double("Enter distance to next stop (km): ");
            double t = read_double("Enter time to next stop (min): ");
            Stop *s = create_stop(name, p, d, t);
            insert_at_position(s, pos);
            printf("Inserted at position %d (or end if pos > length).\n", pos);
        } else if (strcmp(choice, "6") == 0) {
            printf("Enter stop name to delete: ");
            read_line(buf, sizeof(buf));
            if (delete_by_name(buf)) printf("Deleted.\n"); else printf("Stop not found.\n");
        } else if (strcmp(choice, "7") == 0) {
            printf("Enter stop name: ");
            read_line(buf, sizeof(buf));
            Stop *s = find_by_name(buf);
            if (s) printf("Passengers waiting at \"%s\": %d\n", s->name, s->passengers);
            else printf("Stop not found.\n");
        } else if (strcmp(choice, "8") == 0) {
            double td, tt;
            total_distance_time(&td, &tt);
            printf("Total distance of route: %.2f km\nTotal time of route: %.2f minutes\n", td, tt);
        } else if (strcmp(choice, "9") == 0) {
            char a[NAME_LEN], b[NAME_LEN];
            printf("Start stop name: "); read_line(a, sizeof(a));
            printf("End stop name: "); read_line(b, sizeof(b));
            double d=0, t=0;
            if (strcmp(a,b)==0) {
                printf("Same stop. Distance=0, Time=0\n");
            } else if (distance_between(a, b, &d, &t)) {
                printf("Distance from \"%s\" to \"%s\": %.2f km\nTime: %.2f minutes\n", a, b, d, t);
            } else {
                printf("One or both stops not found or unreachable.\n");
            }
        } else if (strcmp(choice, "10") == 0) {
            printf("Filename to save (e.g., route.csv): ");
            read_line(buf, sizeof(buf));
            if (save_to_file(buf)) printf("Saved to %s\n", buf);
        } else if (strcmp(choice, "11") == 0) {
            printf("Filename to load (e.g., route.csv): ");
            read_line(buf, sizeof(buf));
            if (load_from_file(buf)) printf("Loaded from %s\n", buf); else printf("Load failed.\n");
        } else if (strcmp(choice, "12") == 0) {
            populate_sample();
        } else if (strcmp(choice, "0") == 0) {
            printf("Exiting. Freeing memory...\n");
            clear_route();
            return;
        } else {
            printf("Unknown option.\n");
        }
    }
}

int main() {
    printf("Bus Route Simulator (C) — Linked List core logic\n");
    printf("Type 12 in menu to populate sample route for demo.\n");
    menu();
    return 0;
}

