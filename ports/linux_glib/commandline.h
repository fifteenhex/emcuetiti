#pragma once

extern int commandline_port;
extern gchar** commandline_topics;

extern char* commandline_remote_host;
extern int commandline_remote_port;
extern gchar** commandline_remote_topics;
extern int commandline_remote_keepalive;

int commandline_parse(int argc, char** argv);
