#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <time.h>
#include <list>

class ChatMessage {
private:
	char* nik;
	char* text;
	time_t dt;
	char* _str;
	size_t size;
public:

	ChatMessage() : nik{ NULL }, text{ NULL }, dt{ time(NULL) }, _str{ NULL }{}
	ChatMessage(char* nik, char* text) : ChatMessage() {
		SetNik(nik);
		SetText(text);

	}
	char* GetNik() { return nik; }
	char* GetText() { return text; }
	void SetNik(char* nik) {
		if (nik == NULL) return;
		if (this->nik != NULL) delete[] this->nik;
		this->nik = new char[strlen(nik) + 1];
		strcpy(this->nik, nik);
	}
	void SetText(char* text) {
		if (text == NULL) return;
		if (this->text != NULL) delete[] this->text;
		this->text = new char[strlen(text) + 1];
		strcpy(this->text, text);
	}
	time_t GetDT() { return this->dt;}
	void SetDT(time_t dt) {this->dt = dt;}

	bool parseStringDT(char* str) {
		if (str == NULL) return false;
		// looking for TAB symbol
		int tabPosition = -1;
		int len = strlen(str);
		int i = 0;
		while (str[i] != '\t' && i < len) ++i;
		if (i == len) return false;
		tabPosition = i;

		// from 0 to TAB - text
		if (this->text != NULL) delete[] this->text;
		this->text = new char[tabPosition + 1];
		for (i = 0; i < tabPosition; ++i)
			this->text[i] = str[i];
		this->text[tabPosition] = '\0';
		tabPosition++;
		// from TAB to next TAB - nik
		while (str[tabPosition] != '\t'
			&& tabPosition < len) ++tabPosition;
		if (tabPosition == len) return false;

		if (this->nik != NULL) delete[] this->nik;
		this->nik = new char[tabPosition - i + 1];
		for (int j = i + 1; j < tabPosition; ++j)
			this->nik[j - i - 1] = str[j];
		this->nik[tabPosition - i - 1] = '\0';

		// from TAB to END - dt
		char timestamp[16];
		i = tabPosition + 1;
		while (str[i] != '\0') {
			timestamp[i - tabPosition - 1] = str[i];
			i++;
		}
		timestamp[i - tabPosition - 1] = str[i];

		dt = atoi(timestamp);
		
		return true;
	}

	bool parseString(char* str) {
		if (str == NULL) return false;
		// looking for TAB symbol
		int tabPosition = -1;
		int len = strlen(str);
		int i = 0;
		while (str[i] != '\t' && i < len) ++i;
		if (i == len) return false;
		tabPosition = i;

		// from 0 to TAB - text
		if (this->text != NULL) delete[] this->text;
		this->text = new char[tabPosition + 1];
		for (i = 0; i < tabPosition; ++i)
			this->text[i] = str[i];
		this->text[tabPosition] = '\0';

		// from TAB to next TAB - nik

		if (this->nik != NULL) delete[] this->nik;
		this->nik = new char[tabPosition - i + 1];
		for (int j = i + 1; j < len; ++j)
			this->nik[j - i - 1] = str[j];
		this->nik[len - i - 1] = '\0';

		return true;
	}
	
	char* ToString() {
		// text \t nik \t dt
		int text_len = strlen(this->text);
		int nik_len = strlen(this->nik);
		char timestamp[32];
		_itoa(this->dt, timestamp, 10);
		int dt_len = strlen(timestamp);

		if (_str) delete[] _str;

		_str = new char[dt_len + 1 + nik_len + 1 + text_len + 1];
		sprintf(_str, "%s\t%s\t%s",
			this->text, this->nik, timestamp);
		
		return _str;
	}

	char* ToClientString() {

		
		if (this->text == NULL || this->nik == NULL)
			return NULL;

		int text_len = strlen(this->text);
		int nik_len = strlen(this->nik);
		tm* t = new tm;
		localtime_s(t, &this->dt);
		time_t now_t = time(NULL);
		tm* now = localtime(&now_t);

		if (_str) delete[] _str;
		_str = new char[text_len + nik_len + 32];
		if (now->tm_mday == t->tm_mday) {
			sprintf(_str, "%d:%d:%d %s: %s",
				t->tm_hour, t->tm_min, t->tm_sec,
				this->nik, this->text);
		}
		else {
			sprintf(_str, "not today, %d:%d %s: %s",
				t->tm_hour, t->tm_min,
				this->nik, this->text);
		}
		delete t;
		return _str;
	}



	char* ToClientString2() {
		if (this->text == NULL || this->nik == NULL)
			return NULL;

		int text_len = strlen(this->text);
		int nik_len = strlen(this->nik);
		tm* t = new tm;
		localtime_s(t, &this->dt);
		time_t now_t = time(NULL);
		tm* now = localtime(&now_t);

		if (_str) delete[] _str;
		_str = new char[text_len + nik_len + 32];

		if (now->tm_mday == t->tm_mday) {
			sprintf(_str, "%d:%d:%d %s: %s",
				t->tm_hour, t->tm_min, t->tm_sec,
				this->nik, this->text);
		}
		else {
			sprintf(_str, "not today, %d:%d %s: %s",
				t->tm_hour, t->tm_min,
				this->nik, this->text);
		}
		if (t->tm_year == now->tm_year && t->tm_mon == now->tm_mon && t->tm_mday == now->tm_mday) {
			sprintf(_str, "today, %d:%d %s: %s",
				t->tm_hour, t->tm_min,
				this->nik, this->text);
		}
		if (t->tm_year == now->tm_year && t->tm_mon == now->tm_mon && t->tm_mday == now->tm_mday+1) {
			sprintf(_str, "yesterday, %d:%d %s: %s",
				t->tm_hour, t->tm_min,
				this->nik, this->text);
		}
		
		if (t->tm_year == now->tm_year && t->tm_mon == now->tm_mon && t->tm_mday == now->tm_mday <
			t->tm_year == now->tm_year && t->tm_mon == now->tm_mon && t->tm_mday == now->tm_mday + 1){
			
			sprintf(_str, "%d, days ago %d:%d %s: %s",
				now->tm_mday - t->tm_mday, t->tm_hour, t->tm_min,
				this->nik, this->text);
		}
	
		
		
		delete t;
		return _str;
	}
	
	char* UserJoinToServer() {
		if (this->nik == NULL)
			return NULL;
		int nik_len = strlen(this->nik);
		if (_str) delete[] _str;
		_str = new char[2022];
		sprintf(_str, " %s join to server",
			this->nik);
		return _str;
	}


	~ChatMessage() {
		if (_str) delete[] _str;
		if (nik) delete[] nik;
		if (text) delete[] text;
	}

};

