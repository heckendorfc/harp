/*
 *  Copyright (C) 2009  Christian Heckendorf <vaseros@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

int portal(struct commandOption *portalOptions, const char *prefix){
	char *choice=malloc(sizeof(char)*200);
	int x;

	while(printf("%s> ",prefix) && fgets(choice,200,stdin)){
		for(x=0;choice[x] && choice[x]!='\n';x++);
		choice[x]=0;
		for(x=0;portalOptions[x].opt && portalOptions[x].opt!=*choice;x++);
		if(!portalOptions[x].opt){
			switch(*choice){
				case 'q':free(choice);return 0;
				case 'p':free(choice);return 1;
				case '?':
				default:
					printf("Local:\n");
					for(x=0;portalOptions[x].opt;x++)printf("%c\t%s\n",portalOptions[x].opt,portalOptions[x].help);
					printf("\nGlobal:\nq\tQuit\np\tPrevious menu\n?\tPrint help\n\n");
					break;
			}
		}
		else{
			if(!portalOptions[x].function(choice,portalOptions[x].data))break;
		}
	}
	free(choice);
	return 0;
}

