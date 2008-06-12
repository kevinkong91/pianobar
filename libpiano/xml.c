/*
Copyright (c) 2008 Lars-Dominik Braun

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "piano.h"
#include "crypt.h"

/*	parses things like this:
 *	<struct>
 *		<member>
 *			<name />
 *			<value />
 *		</member>
 *		<!-- ... -->
 *	</struct>
 *	@author PromyLOPh
 *	@added 2008-06-03
 *	@param xml node named "struct" (or containing a similar structure)
 *	@param who wants to use this data? callback: content of <name> as
 *			string, content of <value> as xmlNode (may contain other nodes
 *			or text), additional data used by callback(); don't forget
 *			to *copy* data taken from <name> or <value> as they will be
 *			freed soon
 *	@param 
 */
void PianoXmlStructParser (xmlNode *structRoot,
		void (*callback) (char *, xmlNode *, void *), void *data) {

	xmlNode *curNode, *memberNode, *valueNode;
	xmlChar *key;

	/* get all <member> nodes */
    for (curNode = structRoot->children; curNode; curNode = curNode->next) {
        if (curNode->type == XML_ELEMENT_NODE &&
				xmlStrEqual ((xmlChar *) "member", curNode->name)) {
			key = NULL;
			valueNode = NULL;
			/* check children for <name> or <value> */
			for (memberNode = curNode->children; memberNode;
					memberNode = memberNode->next) {
				if (memberNode->type == XML_ELEMENT_NODE) {
					if (xmlStrEqual ((xmlChar *) "name", memberNode->name)) {
						key = memberNode->children->content;
					} else if (xmlStrEqual ((xmlChar *) "value",
							memberNode->name)) {
						valueNode = memberNode->children;
					}
				}
			}
			/* this will ignore empty <value /> nodes, but well... */
			if (key != NULL && valueNode != NULL) {
				(*callback) ((char *) key, valueNode, data);
			}
        }
	}
}

/*	structParser callback; writes userinfo to PianoUserInfo structure
 *	@author PromyLOPh
 *	@added 2008-06-03
 *	@param value identifier
 *	@param value node
 *	@param pointer to userinfo structure
 *	@return nothing
 */
void PianoXmlParseUserinfoCb (char *key, xmlNode *value, void *data) {
	char *valueStr = NULL;
	PianoUserInfo_t *user = data;

	/* some values have subnodes like <boolean> or <string>, just
	 * ignore them... */
	if (value->content != NULL) {
		valueStr = (char *) value->content;
	} else if (value->children != NULL &&
			value->children->content != NULL) {
		 valueStr = (char *) value->children->content;
	}
	/* FIXME: should be continued later */
	if (strcmp ("webAuthToken", key) == 0) {
		user->webAuthToken = strdup (valueStr);
	} else if (strcmp ("authToken", key) == 0) {
		user->authToken = strdup (valueStr);
	} else if (strcmp ("listenerId", key) == 0) {
		user->listenerId = strdup (valueStr);
	}
}

void PianoXmlParseStationsCb (char *key, xmlNode *value, void *data) {
	PianoStation_t *station = data;
	char *valueStr = NULL;

	/* FIXME: copy & waste */
	/* some values have subnodes like <boolean> or <string>, just
	 * ignore them... */
	if (value->content != NULL) {
		valueStr = (char *) value->content;
	} else if (value->children != NULL &&
			value->children->content != NULL) {
		 valueStr = (char *) value->children->content;
	}
	if (strcmp ("stationName", key) == 0) {
		station->name = strdup (valueStr);
	} else if (strcmp ("stationId", key) == 0) {
		station->id = strdup (valueStr);
	}
}

/* FIXME: copy & waste */
void PianoXmlParsePlaylistCb (char *key, xmlNode *value, void *data) {
	PianoSong_t *song = data;
	char *valueStr = NULL;

	/* some values have subnodes like <boolean> or <string>, just
	 * ignore them... */
	if (value->content != NULL) {
		valueStr = (char *) value->content;
	} else if (value->children != NULL &&
			value->children->content != NULL) {
		 valueStr = (char *) value->children->content;
	}
	if (strcmp ("audioURL", key) == 0) {
		/* last 48 chars of audioUrl are encrypted, but they put the key
		 * into the door's lock; dumb pandora... */
		const char urlTailN = 48;
		char *urlTail, *urlTailCrypted = valueStr + (strlen (valueStr) - urlTailN);
		urlTail = PianoDecryptString (urlTailCrypted);
		//printf ("tail is:\t%s\nwas:\t\t%s (%i)\nurl was:\t %s (%i)\n", urlTail, urlTailCrypted, strlen (urlTailCrypted), valueStr, strlen (valueStr));
		song->audioUrl = calloc (strlen (valueStr) + 1, sizeof (char));
		strncpy (song->audioUrl, valueStr, strlen (valueStr) - urlTailN);
		/* FIXME: the key seems to be broken... so ignore 8 x 0x08 postfix;
		 * urlTailN/2 because the encrypted hex string is now decoded */
		strncat (song->audioUrl, urlTail, (urlTailN/2)-8);
		free (urlTail);

	} else if (strcmp ("artistSummary", key) == 0) {
		song->artist = strdup (valueStr);
	} else if (strcmp ("musicId", key) == 0) {
		song->musicId = strdup (valueStr);
	} else if (strcmp ("matchingSeed", key) == 0) {
		song->matchingSeed = strdup (valueStr);
	} else if (strcmp ("userSeed", key) == 0) {
		song->userSeed = strdup (valueStr);
	} else if (strcmp ("focusTraitId", key) == 0) {
		song->focusTraitId = strdup (valueStr);
	} else if (strcmp ("songTitle", key) == 0) {
		song->title = strdup (valueStr);
	} else if (strcmp ("rating", key) == 0) {
		if (strcmp (valueStr, "1") == 0) {
			song->rating = PIANO_RATE_LOVE;
		} else {
			song->rating = PIANO_RATE_NONE;
		}
	}
}

/*	parses server response and updates handle
 *	@author PromyLOPh
 *	@added 2008-06-03
 *	@param piano handle
 *	@param utf-8 string
 *	@return nothing
 */
void PianoXmlParseUserinfo (PianoHandle_t *ph, char *xml) {
	xmlNode *docRoot = NULL;
	xmlDocPtr doc = xmlReadDoc ((xmlChar *) xml, NULL, NULL, 0);

	if (doc == NULL) {
		printf ("whoops... xml parser error\n");
		return;
	}

	docRoot = xmlDocGetRootElement (doc);

	/* <methodResponse> <params> <param> <value> <struct> */
	xmlNode *structRoot = docRoot->children->children->children->children;
	PianoXmlStructParser (structRoot, PianoXmlParseUserinfoCb, &ph->user);

	xmlFreeDoc (doc);
	xmlCleanupParser();
}

/*	parse stations returned by pandora
 *	@author PromyLOPh
 *	@added 2008-06-04
 *	@param piano handle
 *	@param xml returned by pandora
 *	@return nothing
 */
void PianoXmlParseStations (PianoHandle_t *ph, char *xml) {
	/* FIXME: copy & waste */
	xmlNode *docRoot = NULL, *curNode = NULL;
	xmlDocPtr doc = xmlReadDoc ((xmlChar *) xml, NULL, NULL, 0);

	if (doc == NULL) {
		printf ("whoops... xml parser error\n");
		return;
	}

	docRoot = xmlDocGetRootElement (doc);

	/* <methodResponse> <params> <param> <value> <array> <data> */
	xmlNode *dataRoot = docRoot->children->children->children->children->children;
    for (curNode = dataRoot->children; curNode; curNode = curNode->next) {
        if (curNode->type == XML_ELEMENT_NODE &&
				xmlStrEqual ((xmlChar *) "value", curNode->name)) {
			PianoStation_t *tmpStation = calloc (1, sizeof (*tmpStation));
			PianoXmlStructParser (curNode->children,
					PianoXmlParseStationsCb, tmpStation);
			/* start new linked list or append */
			if (ph->stations == NULL) {
				ph->stations = tmpStation;
			} else {
				PianoStation_t *curStation = ph->stations;
				while (curStation->next != NULL) {
					curStation = curStation->next;
				}
				curStation->next = tmpStation;
			}
		}
	}

	xmlFreeDoc (doc);
	xmlCleanupParser();
}

/*	parses playlist; used when searching too
 *	@author PromyLOPh
 *	@added 2008-06-12
 *	@param piano handle
 *	@param xml document
 */
void PianoXmlParsePlaylist (PianoHandle_t *ph, char *xml) {
	/* FIXME: copy & waste */
	xmlNode *docRoot = NULL, *curNode = NULL;
	xmlDocPtr doc = xmlReadDoc ((xmlChar *) xml, NULL, NULL, 0);

	if (doc == NULL) {
		printf ("whoops... xml parser error\n");
		return;
	}

	docRoot = xmlDocGetRootElement (doc);

	/* <methodResponse> <params> <param> <value> <array> <data> */
	xmlNode *dataRoot = docRoot->children->children->children->children->children;
    for (curNode = dataRoot->children; curNode; curNode = curNode->next) {
        if (curNode->type == XML_ELEMENT_NODE &&
				xmlStrEqual ((xmlChar *) "value", curNode->name)) {
			PianoSong_t *tmpSong = calloc (1, sizeof (*tmpSong));
			PianoXmlStructParser (curNode->children,
					PianoXmlParsePlaylistCb, tmpSong);
			/* begin linked list or append */
			if (ph->playlist == NULL) {
				ph->playlist = tmpSong;
			} else {
				PianoSong_t *curSong = ph->playlist;
				while (curSong->next != NULL) {
					curSong = curSong->next;
				}
				curSong->next = tmpSong;
			}
		}
	}

	xmlFreeDoc (doc);
	xmlCleanupParser();
}

/*	parse simple answers like this: <?xml version="1.0" encoding="UTF-8"?>
 *	<methodResponse><params><param><value>1</value></param></params>
 *	</methodResponse>
 *	@author PromyLOPh
 *	@added 2008-06-10
 *	@param xml string
 *	@return
 */
PianoReturn_t PianoXmlParseSimple (char *xml) {
	xmlNode *docRoot = NULL, *curNode = NULL;
	xmlDocPtr doc = xmlReadDoc ((xmlChar *) xml, NULL, NULL, 0);
	PianoReturn_t ret = PIANO_RET_ERR;

	if (doc == NULL) {
		printf ("whoops... xml parser error\n");
		return PIANO_RET_ERR;
	}

	docRoot = xmlDocGetRootElement (doc);

	xmlNode *val = docRoot->children->children->children->children;
	if (xmlStrEqual (val->content, (xmlChar *) "1")) {
		ret = PIANO_RET_OK;
	}

	xmlFreeDoc (doc);
	xmlCleanupParser();

	return ret;
}

/*	xml struct parser callback, used in PianoXmlParseSearchCb
 *	@author PromyLOPh
 *	@added 2008-06-12
 */
void PianoXmlParseSearchArtistCb (char *key, xmlNode *value, void *data) {
	PianoArtist_t *artist = data;
	char *valueStr = NULL;

	/* some values have subnodes like <boolean> or <string>, just
	 * ignore them... */
	if (value->content != NULL) {
		valueStr = (char *) value->content;
	} else if (value->children != NULL &&
			value->children->content != NULL) {
		 valueStr = (char *) value->children->content;
	}

	if (strcmp ("artistName", key) == 0) {
		artist->name = strdup (valueStr);
	} else if (strcmp ("musicId", key) == 0) {
		artist->musicId = strdup (valueStr);
	}
}

/*	callback for xml struct parser used in PianoXmlParseSearch, "switch" for
 *	PianoXmlParseSearchArtistCb and PianoXmlParsePlaylistCb
 *	@author PromyLOPh
 *	@added 2008-06-12
 */
void PianoXmlParseSearchCb (char *key, xmlNode *value, void *data) {
	PianoSearchResult_t *searchResult = data;

	if (strcmp ("artists", key) == 0) {
		xmlNode *curNode;

		/* skip <value><array><data> */
		for (curNode = value->children->children; curNode;
				curNode = curNode->next) {
	        if (curNode->type == XML_ELEMENT_NODE &&
					xmlStrEqual ((xmlChar *) "value", curNode->name)) {

				PianoArtist_t *artist = calloc (1, sizeof (*artist));
				memset (artist, 0, sizeof (*artist));

				PianoXmlStructParser (curNode->children,
						PianoXmlParseSearchArtistCb, artist);

				/* add result to linked list */
				if (searchResult->artists == NULL) {
					searchResult->artists = artist;
				} else {
					PianoArtist_t *curArtist = searchResult->artists;
					while (curArtist->next != NULL) {
						curArtist = curArtist->next;
					}
					curArtist->next = artist;
				}
			}
		}
	} else if (strcmp ("songs", key) == 0) {
		xmlNode *curNode;

		for (curNode = value->children->children; curNode;
				curNode = curNode->next) {
	        if (curNode->type == XML_ELEMENT_NODE &&
					xmlStrEqual ((xmlChar *) "value", curNode->name)) {
				/* FIXME: copy & waste */
				PianoSong_t *tmpSong = calloc (1, sizeof (*tmpSong));
				PianoXmlStructParser (curNode->children,
						PianoXmlParsePlaylistCb, tmpSong);
				/* begin linked list or append */
				if (searchResult->songs == NULL) {
					searchResult->songs = tmpSong;
				} else {
					PianoSong_t *curSong = searchResult->songs;
					while (curSong->next != NULL) {
						curSong = curSong->next;
					}
					curSong->next = tmpSong;
				}
			}
		}
	}
}

/*	parse search result; searchResult is nulled before use
 *	@author PromyLOPh
 *	@added 2008-06-12
 *	@param xml document
 *	@param returns search result
 *	@return nothing yet
 */
void PianoXmlParseSearch (char *searchXml,
		PianoSearchResult_t *searchResult) {
	/* FIXME: copy & waste */
	xmlNode *docRoot = NULL, *curNode = NULL;
	xmlDocPtr doc = xmlReadDoc ((xmlChar *) searchXml, NULL, NULL, 0);

	if (doc == NULL) {
		printf ("whoops... xml parser error\n");
		return;
	}

	docRoot = xmlDocGetRootElement (doc);
	
	xmlNode *structRoot = docRoot->children->children->children->children;
	/* we need a "clean" search result (with null pointers) */
	memset (searchResult, 0, sizeof (*searchResult));
	PianoXmlStructParser (structRoot, PianoXmlParseSearchCb, searchResult);

	xmlFreeDoc (doc);
	xmlCleanupParser();
}

/*	encode reserved xml chars
 *	@author PromyLOPh
 *	@added 2008-06-10
 *	@param encode this
 *	@return encoded string
 */
char *PianoXmlEncodeString (const char *s) {
	char *replacements[] = {"&&amp;", "'&apos;", "\"&quot;", "<&lt;", ">&gt;"};
	char *s_new = NULL;
	unsigned int s_new_n = 0;
	unsigned int i;
	char found = 0;

	s_new_n = strlen (s) + 1;
	s_new = calloc (s_new_n, 1);

	while (*s) {
		found = 0;
		for (i = 0; i < sizeof (replacements) / sizeof (*replacements); i++) {
			if (*s == replacements[i][0]) {
				/* replacements[i]+1 is our replacement, and -1 'cause we
				 * "overwrite" one byte */
				s_new_n += strlen (replacements[i]+1)-1;
				s_new = realloc (s_new, s_new_n);
				strncat (s_new, replacements[i]+1, strlen (replacements[i]+1));
				found = 1;
				/* no need to look for other entities */
				break;
			}
		}
		if (!found) {
			strncat (s_new, s, 1);
		}
		s++;
	}
	return s_new;
}
