#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <predict/predict.h>

void pushoverNotifySolarEclipse()
{
  CURL *curl;
  CURLcode res;

  /* In windows, this will init the winsock stuff */
  curl_global_init(CURL_GLOBAL_ALL);

  /* get a curl handle */
  curl = curl_easy_init();
  if(curl) {
    /* First set the URL that is about to receive our POST. This URL can
       just as well be a https:// URL if that is what should receive the
       data. */
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.pushover.net/1/messages.json");
    /* Now specify the POST data */

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "content-type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    char description[48];
		strcpy(description,"NEARING SOLAR ECLIPSE");

    char postfields[300];

    int q;

    char unescaped_message[200];
    snprintf(unescaped_message,200,"%s", description);
    char *message = curl_easy_escape(curl, unescaped_message,0);
    q=snprintf(postfields, 300, "token={pushover_api_key}&user={pushover_user_key}&message=%s",message);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();

}


int main(int argc, char **argv)
{

	const char *tle_line_1 = "1 25544U 98067A   15129.86961041  .00015753  00000-0  23097-3 0  9998";
	const char *tle_line_2 = "2 25544  51.6464 275.3867 0006524 289.1638 208.5861 15.55704207942078";

	// Create orbit object
	predict_orbital_elements_t *iss = predict_parse_tle(tle_line_1, tle_line_2);
	if (!iss) {
		fprintf(stderr, "Failed to initialize orbit from tle!");
		exit(1);
	}

	// Create observer object
	predict_observer_t *obs = predict_create_observer("Me", 41.6191*M_PI/180.0, -83.5807*M_PI/180.0, 0);
	if (!obs) {
		fprintf(stderr, "Failed to initialize observer!");
		exit(1);
	}

	printf("\e[1;1H\e[2J"); //clear screen

	while (true) {
		printf("\033[0;0H"); //print from start of the terminal

		predict_julian_date_t curr_time = predict_to_julian(time(NULL));

		// Predict ISS
		struct predict_position iss_orbit;
		predict_orbit(iss, &iss_orbit, curr_time);
		printf("ISS: lat=%f, lon=%f, alt=%f\n", iss_orbit.latitude*180.0/M_PI, iss_orbit.longitude*180.0/M_PI, iss_orbit.altitude);

		// Observe ISS
		struct predict_observation iss_obs;
		predict_observe_orbit(obs, &iss_orbit, &iss_obs);
		printf("ISS: azi=%f (rate: %f), ele=%f (rate: %f)\n", iss_obs.azimuth*180.0/M_PI, iss_obs.azimuth_rate*180.0/M_PI, iss_obs.elevation*180.0/M_PI, iss_obs.elevation_rate*180.0/M_PI);

		// Apparent elevation
		double apparent_elevation = predict_apparent_elevation(iss_obs.elevation);
		printf("Apparent ISS elevation: %.2f\n\n", apparent_elevation*180.0/M_PI);

		// Predict and observe MOON
		struct predict_observation moon_obs;
		predict_observe_moon(obs, curr_time, &moon_obs);
		double moon_azimuth_float = moon_obs.azimuth*180.0/M_PI;
		double moon_elevation_float = moon_obs.elevation*180.0/M_PI;
		printf("MOON: %f, %f\n", moon_azimuth_float , moon_elevation_float);

		// Predict and observe SUN
		struct predict_observation sun_obs;
		predict_observe_sun(obs, curr_time, &sun_obs);
		double sun_azimuth_float = sun_obs.azimuth*180.0/M_PI;
		double sun_elevation_float = sun_obs.elevation*180.0/M_PI;
		printf("SUN: %f, %f\n", sun_azimuth_float, sun_elevation_float);

		double eclipse_azimuth_threshold = (moon_azimuth_float > sun_azimuth_float) ? (moon_azimuth_float - sun_azimuth_float) : (sun_azimuth_float - moon_azimuth_float);
		double eclipse_elevation_threshold = (moon_elevation_float > sun_elevation_float) ? (moon_elevation_float - sun_elevation_float) : (sun_elevation_float - moon_elevation_float);

		printf("SOLAR ELCIPSE THRESHOLD: %f, %f\n", eclipse_azimuth_threshold , eclipse_elevation_threshold);

		if(eclipse_azimuth_threshold < 1 && eclipse_elevation_threshold < 1)
		{
			pushoverNotifySolarEclipse();
			printf("SOLAR:\tNEARING SOLAR ECLIPSE");
			exit(0);
		}

		//Sleep
		fflush(stdout);
		usleep(1000000);
	}

	// Free memory
	predict_destroy_orbital_elements(iss);
	predict_destroy_observer(obs);

	return 0;
}
