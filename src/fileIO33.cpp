
#include "fileIO33.h"
#include "model.h"
#include "fleet.h"
#include "fileIOgeneral.h"
#include "message.h"
#include "ss_math.h"
#include "block_pattern.h"

//#define DEBUG_CONTROL

bool read33_dataFile(ss_file *d_file, ss_model *data)
{
    //  SS_Label_Info_2.0 #READ DATA FILE
    QString token;
    QString temp_str;
    QStringList str_lst;
    float temp_float = 0;
    int temp_int = 0, num_input_lines = 0;
    int i = 0, num_vals = 0, total_fleets = 0;
    int n_areas = 0, n_ages = 0, n_genders = 0;
    int units, err_type, year, fleet, obslength;//, season;
//    float obs, err;
//    float month;
    int startYear = 0, endYear = 0;
    int numSeas = 1;

    d_file->setOkay(true);
    if(d_file->open(QIODevice::ReadOnly))
    {
        //  SS_Label_Info_2.1.1 #Read comments
        d_file->seek(0);
//        d_file->reset();
        d_file->resetLineNum();
        d_file->setOkay(true);
        d_file->setStop(false);
        d_file->read_comments();

        //  SS_Label_Info_2.1.2 #Read model time dimensions

        if (d_file->getOkay() && !d_file->getStop()) {
        token = d_file->get_next_value(QString("start year"));
        startYear = token.toInt();
        data->set_start_year (startYear);
        token = d_file->get_next_value(QString("end year"));
        endYear = token.toInt();
        data->set_end_year(endYear);
        }
        if (d_file->getOkay() && !d_file->getStop()) {
        numSeas = d_file->getIntValue(QString("seasons per year"), 1, 12, 1);
        }
        if (d_file->getOkay() && !d_file->getStop()) {
        data->set_num_seasons(numSeas);
        for (i = 1; i <= numSeas; i++)
        {
            temp_float = d_file->getFloatValue(QString("months per season"), .1, 13.9, 12);
            data->set_months_per_season(i, temp_float);
        }
        //  SS_Label_Info_2.1.3 #Set up seasons
        }
        if (d_file->getOkay() && !d_file->getStop()) {
        data->rescale_months_per_season();
        token = d_file->get_next_value(QString("subseasons"));
        temp_int = token.toInt();
        data->set_num_subseasons(temp_int);
        token = d_file->get_next_value(QString("spawning month"));
        temp_float = token.toFloat();
        data->set_spawn_month(temp_float);
        temp_int = d_file->getIntValue(QString("Number of sexes"), -1, 2, 2);
        }
        if (d_file->getOkay() && !d_file->getStop()) {
        n_genders = temp_int;
        data->set_num_genders(n_genders);
        token = d_file->get_next_value(QString("number of ages"));
        temp_int = token.toInt();
        n_ages = temp_int;
        data->set_num_ages(n_ages);
        token = d_file->get_next_value(QString("number of areas"));
        temp_int = token.toInt();
        n_areas = temp_int;
        data->set_num_areas(n_areas);
        data->getPopulation()->Move()->setNumAreas(n_areas);
        //  SS_Label_Info_2.1.5  #Define fleets, surveys and areas
        token = d_file->get_next_value(QString("number of fleets"));
        temp_int = token.toInt();
        if (temp_int < 1) {temp_int = 1;}
        total_fleets = temp_int;
        data->set_num_fleets(total_fleets);
        num_vals = 0;
        }
        if (d_file->getOkay() && !d_file->getStop()) {
        for (i = 0; i < total_fleets; i++)
        {
            Fleet *flt = data->getFleet(i);
            flt->reset();
            flt->setActive(true);
            temp_int = d_file->getIntValue(QString("Fleet type"), 1, 4, 1);
            if (d_file->getOkay()) {
            flt->setTypeInt(temp_int);
            if (temp_int == 2)
                num_vals++;
            temp_float = d_file->get_next_value(QString("Fleet timing")).toFloat();
            flt->setSeasTiming(temp_float);
            temp_int = d_file->getIntValue(QString("Fleet area"), 1, n_areas, 1);
            if (d_file->getOkay()) {
            flt->setArea(temp_int);
            }
            temp_int = d_file->getIntValue(QString("Fleet catch units"), 1, 2, 1);
            if (d_file->getOkay()) {
            flt->setCatchUnits(temp_int);
            }
            temp_int = d_file->getIntValue(QString("Fleet catch multiplier"), 0, 1, 0);
            if (d_file->getOkay()) {
            flt->setCatchMultiplier(temp_int);
            temp_str = d_file->get_next_value(QString("Fleet name"));
            flt->setName(temp_str);
            flt->setNumGenders(data->get_num_genders());
            flt->setNumSeasons(numSeas);
            flt->setStartYear(startYear);
            flt->setTotalYears(data->getTotalYears());
            }
            }
        }
        data->assignFleetNumbers();

        // Read bycatch data, if any
        }
        if (d_file->getOkay() && !d_file->getStop()) {
        for (i = 0; i < num_vals; i++)
        {
            temp_int = d_file->getIntValue(QString("Fleet Index"), 1, total_fleets, 1);
            if (d_file->getOkay()) {
            Fleet *flt = data->getFleet(temp_int - 1);
            temp_int = d_file->getIntValue(QString("Include in MSY"), 1, 2, 1);
            flt->setBycatchDead(temp_int);
            temp_int = d_file->getIntValue(QString("F multiplier"), 1, 3, 1);
            flt->setBycatchF(temp_int);
            temp_str = d_file->get_next_value(QString("First year"));
            flt->setBycFirstYr(temp_str);
            temp_str = d_file->get_next_value(QString("Last year"));
            flt->setBycLastYr(temp_str);
            temp_str = d_file->get_next_value(QString("unused"));
            flt->setBycUnused(temp_str);
            }
        }
        }
        //  ProgLabel_2.2  Read CATCH amount by fleet
        // Catch
        if (d_file->getOkay() && !d_file->getStop()) {
        num_vals = total_fleets;
        bool info = false;
        do {
            bool neg = false;
            int seas = 0;
            double ctch = 0;
            str_lst.clear();
            str_lst.append(d_file->get_next_value(QString("year")));
            str_lst.append(d_file->get_next_value(QString("season")));
            str_lst.append(d_file->get_next_value(QString("fleet")));
            str_lst.append(d_file->get_next_value(QString("catch")));
            str_lst.append(d_file->get_next_value(QString("catch_se")));
            year   = str_lst.at(0).toInt();
            if (year == END_OF_LIST)
                break;
            fleet  = abs(str_lst.at(2).toInt());
            if (fleet < 0)
                neg = true;
            fleet  = d_file->checkIntValue(fleet, QString("Fleet number for catch"), 1, num_vals, 1);
            if (neg)
                str_lst[2] = QString("-%1").arg(fleet);
            else
                str_lst[2] = QString("%1").arg(fleet);
            seas   = str_lst.at(1).toInt();
            if (seas > numSeas) {
                seas = numSeas;
                if (!info) {
                QString msg("Catch observation season exceeds number of seasons. This will be changed to the last season.");
                QMessageBox::information(nullptr, "Reading data file Catch data", msg);
                info = true;
                }
                str_lst[1] = QString::number(seas);
            }
            ctch = str_lst.at(3).toDouble();
            if (year == -999)
                data->getFleet(fleet - 1)->set_catch_equil(seas, ctch);
            data->getFleet(fleet - 1)->addCatchObservation(str_lst);
        } while (year != END_OF_LIST);

        //  SS_Label_Info_2.3 #Read fishery CPUE, effort, and Survey index or abundance
        // before we record abundance, get units and error type for all fleets
        }
        if (d_file->getOkay() && !d_file->getStop()) {
        for (i = 0; i < total_fleets; i++)
        {
            Fleet *flt = data->getFleet(i);
            fleet = abs(d_file->get_next_value(QString("Abund fleet num")).toInt()); // fleet number
            if (fleet != (i + 1))
                d_file->error(QString("Fleet number does not match."));
            units = d_file->get_next_value(QString("Abund units")).toInt(); // units
            flt->setAbundUnits(units);
            err_type = d_file->get_next_value(QString("Abund error dist")).toInt(); // err_flt->setAbundErrType(err_type);
            flt->setAbundErrType(err_type);
            temp_int = d_file->getIntValue(QString("Abund enable SD_Report"), 0, 1, 0);
            flt->setSDReport(temp_int);
        }
        }
        if (d_file->getOkay() && !d_file->getStop()) {
        // here are the abundance numbers
        do
        {    // year, month, fleet_number, observation, error
            str_lst.clear();
            str_lst.append(d_file->get_next_value(QString("Abund year")));
            str_lst.append(d_file->get_next_value(QString("Abund month")));
            str_lst.append(d_file->get_next_value(QString("Abund fleet")));
            str_lst.append(d_file->get_next_value(QString("Abund obs")));
            str_lst.append(d_file->get_next_value(QString("Abund err")));
            year = str_lst.at(0).toInt();
            if (year == END_OF_LIST)
                break;

            fleet = abs(str_lst.at(2).toInt());
            fleet = d_file->checkIntValue(fleet, QString("Fleet number"), 1, total_fleets, 1);
            data->getFleet(fleet - 1)->addAbundanceObs(str_lst);

        } while (year != END_OF_LIST);
        }
        if (d_file->getOkay() && !d_file->getStop()) {
        //  SS_Label_Info_2.4 #read Discard data
        token = d_file->get_next_value("num fleets with discard");
        num_vals = token.toInt();
        if (num_vals > 0)
        {
            for (i = 0; i < num_vals; i++)
            {
                fleet = abs(d_file->get_next_value(QString("discard fleet")).toInt()) - 1;
                units = d_file->get_next_value(QString("discard units")).toInt();
                err_type = d_file->get_next_value(QString("discard error dist")).toInt();
                data->getFleet(fleet)->setDiscardUnits(units);
                data->getFleet(fleet)->setDiscardErrType(err_type);
            }
            // observations
            do
            {    // year, month, fleet_number, observation, error
                str_lst.clear();
                str_lst.append(d_file->get_next_value(QString("Discard year")));
                str_lst.append(d_file->get_next_value(QString("Discard month")));
                str_lst.append(d_file->get_next_value(QString("Discard fleet")));
                str_lst.append(d_file->get_next_value(QString("Discard obs")));
                str_lst.append(d_file->get_next_value(QString("Discard err")));
                year = str_lst.at(0).toInt();
                if (year == END_OF_LIST)
                    break;

                fleet = abs(str_lst.at(2).toInt());
                fleet = d_file->checkIntValue(fleet, QString("Fleet number"), 1, total_fleets, 1);
                data->getFleet(fleet - 1)->addDiscard(str_lst);
                if (str_lst.last().compare(QString("EOF")) == 0)
                    return false;
            } while (year != END_OF_LIST);
        }

        }
        if (d_file->getOkay() && !d_file->getStop()) {
        //  SS_Label_Info_2.5 #Read Mean Body Weight data
        //  note that syntax for storing this info internal is done differently than for surveys and discard
        temp_int = d_file->getIntValue(QString("use mean body weight"), 0, 1, 0);
        data->setUseMeanBwt(temp_int);
        if (temp_int > 0)
        {
            temp_int = d_file->get_next_value(QString("mean body wt deg of freedom")).toInt();
            for (i = 0; i < total_fleets; i++)
                data->getFleet(i)->setMbwtDF(temp_int);
            do
            {    // year, month, fleet_number, partition, type, obs, stderr
                str_lst.clear();
                for (int j = 0; j < 7; j++)
                    str_lst.append(d_file->get_next_value(QString("mean body wt data")));
                year = str_lst.at(0).toInt();
                if (year == END_OF_LIST)
                    break;
                fleet = abs(str_lst.at(2).toInt());
                fleet = d_file->checkIntValue(fleet, QString("Fleet number for body wt data"), 1, total_fleets, 1);
                data->getFleet(fleet - 1)->addMbwtObservation(str_lst);
                if (str_lst.at(4).compare(QString("EOF")) == 0)
                    return false;
            } while (year != END_OF_LIST);
        }

        }
        if (d_file->getOkay() && !d_file->getStop()) {
        //  SS_Label_Info_2.6 #Setup population Length bins
        ss_growth *grow = data->getPopulation()->Grow();
        temp_int = d_file->getIntValue(QString("Length comp alt bin method"), 1, 3, 1);
        grow->setGrowthBinMethod(temp_int);
        switch (temp_int)
        {
        case 1:  // same as data bins - set after data bins
            break;
        case 2:  // generate from min, max, width
            float min, max, width;
            token = d_file->get_next_value(QString("Length comp bin width"));
            width = token.toFloat();
            grow->setGrowthBinStep(width);
            token = d_file->get_next_value(QString("Length comp alt bin min"));
            min = token.toFloat();
            grow->setGrowthBinMin(min);
            token = d_file->get_next_value(QString("Length comp alt bin max"));
            max = token.toFloat();
            grow->setGrowthBinMax(max);
            grow->generateGrowthBins();
            break;
        case 3:  // read vector
            str_lst.clear();
            temp_int = d_file->get_next_value(QString("Length comp num alt bins")).toInt();
            grow->setNumGrowthBins(temp_int);
            for (int j = 0; j < temp_int; j++)
                str_lst.append(d_file->get_next_value(QString("Length comp alt bin")));
            grow->setGrowthBins(str_lst);
            break;
        }
        }

        //  SS_Label_Info_2.7 #Start length data section
        if (d_file->getOkay() && !d_file->getStop()) {
        compositionLength *l_data = data->get_length_composition();
        if (l_data == nullptr) {
            l_data = new compositionLength(data);
            data->set_length_composition(l_data);
        }
        int num_len_bins = 2;
        temp_int = d_file->getIntValue(QString("Use length comp data?"), 0, 1, 1);
        data->setUseLengthComp(temp_int);
        if (temp_int == 1)
        {
            for (i = 0; i < total_fleets; i++)
            {
                data->getFleet(i)->setLengthMinTailComp(d_file->get_next_value(QString("min tail comp")));
                data->getFleet(i)->setLengthAddToData(d_file->get_next_value(QString("add to data")));
                temp_int = d_file->get_next_value(QString("combine genders")).toInt();
                data->getFleet(i)->setLengthCombineGen(temp_int);
                temp_int = d_file->get_next_value(QString("compress bins")).toInt();
                data->getFleet(i)->setLengthCompressBins(temp_int);
                temp_int = d_file->get_next_value(QString("error dist")).toInt();
                data->getFleet(i)->setLengthCompError(temp_int);
                temp_int = d_file->get_next_value(QString("error parameter")).toInt();
                data->getFleet(i)->setLengthCompErrorParm(temp_int);
                temp_float = d_file->get_next_value(QString("min sample size")).toFloat();
                data->getFleet(i)->setLengthMinSampleSize(temp_float);
            }
            num_len_bins = d_file->get_next_value(QString("Length comp number bins")).toInt();
            l_data->setNumberBins(num_len_bins);
            for (int j = 0; j < total_fleets; j++)
                data->getFleet(j)->setLengthNumBins(num_len_bins);
            str_lst.clear();
            for (i = 0; i < num_len_bins; i++)
            {
                str_lst.append(d_file->get_next_value(QString("Length comp bin")));
            }
            l_data->setBins(str_lst);
            // set pop bins if method = 1
            ss_growth *grow = data->getPopulation()->Grow();
            if (grow->getGrowthBinMethod() == 1)
            {
                grow->setNumGrowthBins(str_lst.count());
                grow->setGrowthBins(str_lst);
            }

            //  SS_Label_Info_2.7.4 #Read Length composition data
            obslength = data->getFleet(0)->getLengthObsLength();
            do
            {
                str_lst.clear();
                for (int j = 0; j < obslength; j++)
                {
                    token = d_file->get_next_value(QString("Length comp data"));
                    str_lst.append(token);
                }
                if (str_lst.at(0).toInt() == END_OF_LIST)
                    break;
                if (str_lst.at(0).compare("EOF") == 0)
                    return false;
                temp_int = abs(str_lst.at(2).toInt());
                data->getFleet(temp_int - 1)->addLengthObservation(str_lst);// getLengthObs.addObservation(data);
            } while (str_lst.at(0).toInt() != END_OF_LIST);
            data->set_length_composition(l_data);
        }
        else {
            num_len_bins = 2;
        }

        }

        //  SS_Label_Info_2.8 #Start age composition data section
        if (d_file->getOkay() && !d_file->getStop()) {
        compositionAge *a_data = data->get_age_composition();
        if (a_data == nullptr)
            a_data = new compositionAge ();
        token = d_file->get_next_value(QString("age comp num bins"));
        temp_int = token.toInt();
        a_data->setNumberBins(temp_int);
        for (i = 0; i < total_fleets; i++)
        {
            data->getFleet(i)->setAgeNumBins(temp_int);
            data->getFleet(i)->setSaaNumBins(temp_int);
        }
        str_lst.clear();
        for (i = 0; i < temp_int; i++)
        {
            token = d_file->get_next_value(QString("age comp bin"));
            str_lst.append(token);
        }
        a_data->setBins(str_lst);
        if (temp_int > 0) {
            token = d_file->get_next_value(QString("age comp num error defs"));
            temp_int = token.toInt();
            a_data->set_num_error_defs(temp_int);
            for (i = 0; i < temp_int; i++)
            {
                int numAges = data->get_num_ages();
                str_lst.clear();
                for (int j = 0; j <= numAges; j++)
                    str_lst.append(d_file->get_next_value());
                a_data->set_error_def_ages(i, str_lst);
                str_lst.clear();
                for (int j = 0; j <= numAges; j++)
                    str_lst.append(d_file->get_next_value());
                a_data->set_error_def(i, str_lst);
                if (a_data->get_error_def(i).at(0).toInt() < 0)
                {
                    if (a_data->getUseAgeKeyZero() < 0)
                        a_data->setUseAgeKeyZero(i);
                }
            }
            for (i = 0; i < total_fleets; i++)
            {
                str_lst.clear();
                data->getFleet(i)->setAgeMinTailComp(d_file->get_next_value(QString("min tail comp")));
                data->getFleet(i)->setAgeAddToData(d_file->get_next_value(QString("add to data")));
                temp_int = d_file->get_next_value(QString("combine genders")).toInt();
                data->getFleet(i)->setAgeCombineGen(temp_int);
                temp_int = d_file->get_next_value(QString("compress bins")).toInt();
                data->getFleet(i)->setAgeCompressBins(temp_int);
                temp_int = d_file->get_next_value(QString("error")).toInt();
                data->getFleet(i)->setAgeCompError(temp_int);
                temp_int = d_file->get_next_value(QString("error parm")).toInt();
                data->getFleet(i)->setAgeCompErrorParm(temp_int);
                temp_float = d_file->get_next_value(QString("min sample size")).toFloat();
                data->getFleet(i)->setAgeCompMinSampleSize(temp_float);
            }

            temp_int = d_file->getIntValue(QString("Age comp alt bin method"), 1, 3, 1);
            a_data->setAltBinMethod(temp_int);

            //  SS_Label_Info_2.8.2 #Read Age data
            obslength = data->getFleet(0)->getAgeObsLength();
            do
            {
                str_lst.clear();
                for (int j = 0; j < obslength; j++)
                {
                    token = d_file->get_next_value(QString("Age data"));
                    str_lst.append(token);
                    if (token.contains("END_OF_LIST"))
                    {
                        d_file->skip_line();
                        break;
                    }
                }
                if (str_lst.at(0).toInt() == END_OF_LIST)
                    break;
                if (str_lst.at(0).compare(QString("EOF")) == 0)
                    return false;
                fleet = abs(str_lst.at(2).toInt());
                fleet = d_file->checkIntValue(fleet, QString("Fleet Number"), 1, total_fleets, 1);
                data->getFleet(fleet - 1)->addAgeObservation(str_lst);
            } while (str_lst.at(0).toInt() != END_OF_LIST);
        }
        }

        //  SS_Label_Info_2.9 #Read mean Size_at_Age data
        if (d_file->getOkay() && !d_file->getStop()) {
        temp_int = d_file->get_next_value().toInt();
        data->setUseMeanSAA(temp_int);
        if (temp_int > 0)
        {
            obslength = data->getFleet(0)->getSaaObservation(0).count();
            do
            {
                str_lst.clear();
                for (int j = 0; j < obslength; j++)
                {
                    token = d_file->get_next_value(QString("mean Size_at_Age"));
                    str_lst.append(token);
                }
                year = str_lst.at(0).toInt();
                if (year == END_OF_LIST)
                    break;
                if (str_lst.at(0).compare(QString("EOF")) == 0)
                    return false;
                fleet = abs(str_lst.at(2).toInt());
                fleet = d_file->checkIntValue(fleet, QString("Fleet Number"), 1, total_fleets, 1);
                data->getFleet(fleet - 1)->addSaaObservation(str_lst);
            } while (year != END_OF_LIST);
        }
        }

        //  SS_Label_Info_2.10 #Read environmental data that will be used to modify processes and expected values
        if (d_file->getOkay() && !d_file->getStop()) {
        temp_int = d_file->get_next_value(QString("num env vars")).toInt();
        data->setNumEnvironVars (temp_int);
        data->setNumEnvironVarObs(0);
        if (temp_int > 0)
        {
            obslength = data->getEnvVariables()->columnCount();
            do
            {
                str_lst.clear();
                for(int j = 0; j < obslength; j++)
                {
                    str_lst.append(d_file->get_next_value(QString("env var data")));
                }
                temp_int = str_lst.at(0).toInt();
                if (temp_int != END_OF_LIST)
                    data->addEnvironVarObs (str_lst);
                if (str_lst.at(0).compare(QString("EOF")) == 0)
                    return false;
            } while (temp_int != END_OF_LIST);
        }
        }

        //  SS_Label_Info_2.11 #Start generalized size composition section
        if (d_file->getOkay() && !d_file->getStop()) {
        num_vals = d_file->get_next_value().toInt();
        if (num_vals < 0)
        {
            data->setDMError(-1);
            num_vals = d_file->get_next_value().toInt();
        }
        else
        {
            data->setDMError(0);
        }
        if (num_vals > 0)
        {
            for (i = 0; i < num_vals; i++)
            {
                compositionGeneral *cps = new compositionGeneral (data);
                cps->setNumber(i+1);
                data->addGeneralCompMethod(cps);
            }
            for (int j = 0; j < total_fleets; j++)
                data->getFleet(j)->setGenModelTotal(num_vals);
            // Sizefreq N bins
            for (i = 0; i < num_vals; i++)
            {
                temp_int = d_file->get_next_value().toInt();
                data->getGeneralCompMethod(i)->setNumberBins(temp_int);
                for (int j = 0; j < total_fleets; j++)
                    data->getFleet(j)->setGenNumBins(i, temp_int);
            }
            // Sizefreq units (1=biomass, 2=numbers)
            for (i = 0; i < num_vals; i++)
            {
                temp_int = d_file->getIntValue("Sizefreg units (1=bio/2=num)", 1, 2, 1);
                data->getGeneralCompMethod(i)->setUnits(temp_int);
            }
            // Sizefreq scale
            for (i = 0; i < num_vals; i++)
            {
                temp_int = d_file->getIntValue("Sizefreq scale (1=kg/2=lbs/3=cm/4=inches)", 1, 4, 1);
                data->getGeneralCompMethod(i)->setScale(temp_int);
                if (temp_int > 2 && data->getGeneralCompMethod(i)->getUnits() == 1)
                    QMessageBox::warning(nullptr, QString("Fatal combination - SizeFreq scale and units "),
                          QString("Error: cannot accumulate biomass into length-based szfreq scale for method: %1").arg(i));
            }
            // Sizefreq mincomp
            for (i = 0; i < num_vals; i++)
            {
                temp_float = d_file->getFloatValue("Sizefreq small constant to add to comps", 0.0, .0001, .0000001);
                data->getGeneralCompMethod(i)->setMinComp(temp_float);
            }
            // Sizefreq num obs per method
            for (i = 0; i < num_vals; i++)
            {
                temp_int = d_file->get_next_value().toInt();
                data->getGeneralCompMethod(i)->setNumberObs(temp_int);
            }
            if (data->getDMError() == -1)
            {
                // SizeFreq Comp_Error: type
                for (i = 0; i < num_vals; i++)
                {
                    temp_int = d_file->getIntValue("Comp_Error: 0=multinomial, 1=dirichlet using Theta*n, 2=dirichlet using beta, 3=MV_Tweedie", 0, 3, 0);
                    data->getGeneralCompMethod(i)->setCompErrorType(temp_int);
                }
                // SizeFreq Comp_Error: index
                for (i = 0; i < num_vals; i++)
                {
                    temp_int = d_file->getIntValue("Comp_Error:  index for dirichlet or MV_Tweedie", 0, 1, 0);
                    data->getGeneralCompMethod(i)->setCompErrorIndex(temp_int);
                }
            }
            else
            {
                for (i = 0; i < num_vals; i++)
                {
                    data->getGeneralCompMethod(i)->setCompErrorType(0);
                    data->getGeneralCompMethod(i)->setCompErrorIndex(0);
                }
            }
            // SizeFreq bins
            for (i = 0; i < num_vals; i++)
            {
                compositionGeneral *cps = data->getGeneralCompMethod(i);
                str_lst.clear();
                for (int j = 0; j < cps->getNumberBins(); j++)
                {
                    str_lst.append(d_file->get_next_value());
                }
                cps->getBinsModel()->setRowData(0, str_lst);
            }
            num_input_lines = 0;
            for (i = 0; i < num_vals; i++)
            {
                compositionGeneral *cps = data->getGeneralCompMethod(i);
                num_input_lines += cps->getNumberObs();
            }
            for (i = 0; i < num_input_lines; i++)
            {
                str_lst.clear();
                str_lst.append(d_file->get_next_value());
                temp_int = str_lst.at(0).toInt();
                obslength = data->getFleet(0)->getGenObsLength(temp_int-1) - 1;

                for (int k = 0; k < obslength; k++)
                {
                    str_lst.append(d_file->get_next_value());
                }

                fleet = abs(str_lst.at(3).toInt());
                data->getFleet(fleet-1)->addGenObservation(temp_int-1, str_lst);
            }
        }
        }

        //  SS_Label_Info_2.12 #Read tag release and recapture data
        if (d_file->getOkay() && !d_file->getStop()) {
        temp_int = d_file->getIntValue(QString("Do tags"), 0, 2, 0);
        data->setDoTags(temp_int);
        if (data->getDoTags())
        {
            temp_int = d_file->get_next_value().toInt();
            data->setNumTagGroups(temp_int);
            temp_int = d_file->get_next_value().toInt();
            data->setNumTagRecaps(temp_int);

            temp_int = d_file->get_next_value().toInt();
            data->setTagLatency(temp_int);
            temp_int = d_file->get_next_value().toInt();
            data->setTagMaxPeriods(temp_int);
            // release data
            for (i = 0; i < data->getNumTagGroups(); i++)
            {
                str_lst.clear();
                for (int j = 0; j < 8; j++)
                    str_lst.append(d_file->get_next_value());
                data->setTagObservation(i, str_lst);
            }
            // recapture data
            bool last_tag = false;
            int tag = 0, tag_num = data->getNumTagGroups();
            num_vals = data->getNumTagRecaps();
            for (int i = 0; i < num_vals; i++)
            {
                token = d_file->get_next_value("tag");
                tag = token.toInt();
                if (last_tag && tag != tag_num) {
                    d_file->return_token(token);
                    break;
                }
                str_lst.clear();
                str_lst.append(token);
                for (int j = 1; j < 5; j++)
                    str_lst.append(d_file->get_next_value());
                fleet = abs(str_lst.at(3).toInt());
                data->getFleet(fleet - 1)->addRecapObservation(str_lst);
                if (tag_num == tag)
                    last_tag = true;
            }
            for (int i = 0; i < data->get_num_fleets(); i++)
            {
                num_input_lines = data->getFleet(i+1)->getRecapModel()->rowCount();
                data->getFleet(i+1)->setRecapNumEvents(num_input_lines);
            }
        }
        }

        //  SS_Label_Info_2.13 #Morph composition data
        if (d_file->getOkay() && !d_file->getStop()) {
        temp_int = d_file->getIntValue(QString("Do morph composition"), 0, 1, 0);
        data->setDoMorphComp(temp_int == 1);
        total_fleets = data->get_num_fleets();
        if (data->getDoMorphComp())
        {
            compositionMorph *mcps = new compositionMorph();
            data->set_morph_composition (mcps);
            num_input_lines = d_file->get_next_value().toInt(); // num observations
            mcps->setNumberObs(num_input_lines);
            temp_int = d_file->get_next_value().toInt(); // number of platoons
            for (int j = 0; j < total_fleets; j++)
                data->getFleet(j)->setMorphNumMorphs(temp_int);
            temp_str = d_file->get_next_value();         // min compression
            for (int j = 0; j < total_fleets; j++)
                data->getFleet(j)->setMorphMinTailComp(temp_str);

            obslength = data->getFleet(0)->getMorphObsLength() + 1;
            for (i = 0; i < num_input_lines; i++)
            {
                str_lst.clear();
                for (int j = 0; j < obslength; j++)
                {
                    str_lst.append(d_file->get_next_value());
                }
                temp_int = str_lst.at(1).toInt();
                if (temp_int < 0)
                {
                    d_file->error("Negative month not allowed since superperiods not implemented. Changed.");
                    str_lst[1] = QString::number(-temp_int);
                }
                temp_int = abs(str_lst.at(2).toInt());
                data->getFleet(temp_int - 1)->addMorphObservation(str_lst);
            }
        }
        temp_int = d_file->getIntValue(QString("Do dataread for selectivity priors"), 0, 1, 0);
        data->setReadSelectivityPriors(temp_int == 1? true: false);
        }
        //  SS_Label_Info_2.14 #End of datafile indicator
        temp_int = d_file->get_next_value().toInt();
        if (temp_int != END_OF_DATA)
        {
            d_file->error(QString("Found incorrect end of data marker."));
        }

        d_file->close();
    }
    else
    {
        d_file->error(QString("Data file does not exist or is unreadable."));
    }
    return d_file->getOkay();
}

int write33_dataFile(ss_file *d_file, ss_model *data)
{
    QString temp_str, line;
    QStringList str_lst, tmp_lst;
    int i, j, chars = 0;
    int temp_int = 0, num, num_lines;
//    float temp_float = 0.0;
    int total_fleets = data->getNumActiveFleets();
    Fleet *flt;

    if(d_file->open(QIODevice::WriteOnly))
    {
        chars += writeVersionComment(d_file);
        chars += d_file->write_comments();

        chars += d_file->writeline(QString ("#_observed data: "));
        chars += d_file->writeline(QString("#"));
        chars += d_file->write_val(data->get_start_year(), 1, QString ("StartYr"));
        chars += d_file->write_val(data->get_end_year(), 1, QString ("EndYr"));
        chars += d_file->write_val(data->get_num_seasons(), 1, QString ("Nseas"));

        line.clear();
        for (i = 0; i < data->get_num_seasons(); i++)
            line.append (QString(" %1").arg
                         (QString::number(data->get_months_per_season(i))));
        line.append (" #_months/season");
        chars += d_file->writeline (line);
        chars += d_file->write_val(data->get_num_subseasons(),1,QString ("Nsubseasons (even number, minimum is 2)"));
        chars += d_file->write_val(data->get_spawn_month(), 1, QString ("spawn_month"));
        chars += d_file->write_val(data->get_num_genders(), 1, QString ("Ngenders: 1, 2, -1  (use -1 for 1 sex setup with SSB multiplied by female_frac parameter)"));
        chars += d_file->write_val(data->get_num_ages(), 1, QString ("Nages=accumulator age, first age is always age 0"));
        chars += d_file->write_val(data->get_num_areas(), 1, QString ("Nareas"));
        chars += d_file->write_val(data->get_num_fleets(), 1, QString ("Nfleets (including surveys)"));

        line = QString("#_fleet_type: 1=catch fleet; 2=bycatch only fleet; 3=survey; 4=ignore ");
        chars += d_file->writeline (line);
        line = QString("#_sample_timing: -1 for fishing fleet to use season-long catch-at-age for observations, or 1 to use observation month;  (always 1 for surveys)");
        chars += d_file->writeline (line);
        line = QString("#_fleet_area:  area the fleet/survey operates in ");
        chars += d_file->writeline (line);
        line = QString("#_units of catch:  1=bio; 2=num (ignored for surveys; their units read later)");
        chars += d_file->writeline (line);
        line = QString("#_catch_mult: 0=no; 1=yes");
        chars += d_file->writeline (line);
        line = QString("#_rows are fleets");
        chars += d_file->writeline (line);
        line = QString("#_fleet_type fishery_timing area catch_units need_catch_mult fleetname");
        chars += d_file->writeline (line);

        for (i = 1; i <= total_fleets; i++)
        {
            flt = data->getActiveFleet(i);
            str_lst.clear();
            str_lst.append(QString::number(flt->getTypeInt()));
            str_lst.append(QString::number(flt->getSeasTiming()));
            str_lst.append(QString::number(flt->getArea()));
            str_lst.append(QString::number(flt->getCatchUnits()));
            str_lst.append(QString::number(flt->getCatchMultiplier()));
            str_lst.append(flt->getName());
            chars += d_file->write_vector(str_lst, 2, QString::number(i));
        }
        line = QString("#Bycatch_fleet_input_goes_next");
        chars += d_file->writeline (line);
        line = QString("#a:  fleet index");
        chars += d_file->writeline (line);
        line = QString("#b:  1=include dead bycatch in total dead catch for F0.1 and MSY optimizations and forecast ABC; 2=omit from total catch for these purposes (but still include the mortality)");
        chars += d_file->writeline (line);
        line = QString("#c:  1=Fmult scales with other fleets; 2=bycatch F constant at input value; 3=bycatch F from range of years");
        chars += d_file->writeline (line);
        line = QString("#d:  F or first year of range");
        chars += d_file->writeline (line);
        line = QString("#e:  last year of range");
        chars += d_file->writeline (line);
        line = QString("#f:  not used");
        chars += d_file->writeline (line);
        line = QString("# a   b   c   d   e   f ");
        chars += d_file->writeline (line);
        for (i = 0; i < total_fleets; i++)
        {
            flt = data->getActiveFleet(i);
            if (flt->getTypeInt() == 2)
            {
                str_lst.clear();
                str_lst.append(QString::number(i));
                str_lst.append(QString::number(flt->getBycatchDead()));
                str_lst.append(QString::number(flt->getBycatchF()));
                str_lst.append(flt->getBycFirstYr());
                str_lst.append(flt->getBycLastYr());
                str_lst.append(flt->getBycUnused());
                chars += d_file->write_vector(str_lst, 5, flt->getName());
            }
        }

        chars += d_file->writeline(QString("#_Catch data: yr, seas, fleet, catch, catch_se"));
        chars += d_file->writeline(QString("#_catch_se:  standard error of log(catch)"));
        chars += d_file->writeline(QString("#_NOTE:  catch data is ignored for survey fleets"));
        for (i = 1; i <= total_fleets; i++)
        {
            flt = data->getActiveFleet(i);
            if (flt->getTypeInt() < 3)
            {
                num_lines = flt->getCatchModel()->rowCount();
                for (int j = 0; j <= num_lines; j++)
                {
                    str_lst = flt->getCatchObservation(j);
                    if (str_lst.at(0).isEmpty())
                        break;
                    chars += d_file->write_vector(str_lst, 2);
                }
            }
        }
        line = QString("-9999 0 0 0 0");
        chars += d_file->writeline (line);
        chars += d_file->writeline ("#");

        // CPUE Abundance
        line = QString(" #_CPUE_and_surveyabundance_observations");
        chars += d_file->writeline(line);
        line = QString("#_Units:  0=numbers; 1=biomass; 2=F; 30=spawnbio; 31=recdev; 32=spawnbio*recdev; 33=recruitment; 34=depletion(&see Qsetup); 35=parm_dev(&see Qsetup)");
        chars += d_file->writeline(line);
        line = QString("#_Errtype:  -1=normal; 0=lognormal; >0=T");
        chars += d_file->writeline(line);
        line = QString("#_SD_Report: 0=no sdreport; 1=enable sdreport");
        chars += d_file->writeline(line);
        line = QString("#_Fleet Units Errtype SD_Report");
        chars += d_file->writeline(line);
        for (i = 1; i <= total_fleets; i++)
        {
            flt = data->getActiveFleet(i);
            line = QString(QString("%1 %2 %3 %4 # %5").arg (
                   QString::number(i),
                   QString::number(flt->getAbundUnits()),
                   QString::number(flt->getAbundErrType()),
                   QString::number(flt->getSDReport()),
                   flt->getName()));
            chars += d_file->writeline (line);
        }

        line = QString("#_yr month fleet obs stderr");
        chars += d_file->writeline (line);
        for (i = 1; i <= total_fleets; i++)//data->num_fisheries();i < data->num_fleets(); i++)
        {
            flt = data->getActiveFleet(i);
            int num_lines = flt->getAbundanceCount();
            for (int j = 0; j < num_lines; j++)
            {
                QStringList abund (flt->getAbundanceObs(j));
                if (!abund.at(0).isEmpty())
                {
                    chars += d_file->write_vector(abund, 4, flt->getName());
                }
            }
        }
        line = QString("-9999 1 1 1 1 # terminator for survey observations ");
        chars += d_file->writeline (line);

        chars += d_file->writeline ("#");

        // discard
        temp_int = data->fleet_discard_count();
        line = QString (QString ("%1 #_N_fleets_with_discard").arg (temp_int));
        chars += d_file->writeline (line);
        line = QString("#_discard_units (1=same_as_catchunits(bio/num); 2=fraction; 3=numbers)");
        chars += d_file->writeline (line);
        line = QString("#_discard_errtype:  >0 for DF of T-dist(read CV below); 0 for normal with CV; -1 for normal with se; -2 for lognormal; -3 for trunc normal with CV");
        chars += d_file->writeline (line);
        line = QString ("# note: only enter units and errtype for fleets with discard ");
        chars += d_file->writeline (line);
        line = QString ("# note: discard data is the total for an entire season, so input of month here must be to a month in that season");
        chars += d_file->writeline (line);
        line = QString ("#_Fleet units errtype");
        chars += d_file->writeline (line);
        for (i = 1; i <= total_fleets; i++)
        {
            flt = data->getActiveFleet(i);
            if (flt->getDiscardCount() > 0)
            {
                line = QString(QString("%1 %2 %3 # %4").arg(
                            QString::number(i),
                            QString::number(flt->getDiscardUnits()),
                            QString::number(flt->getDiscardErrType()),
                            flt->getName()));
                chars += d_file->writeline (line);
            }
        }

        num = data->fleet_discard_obs_count();
        line = QString("#_yr month fleet obs stderr");
        chars += d_file->writeline (line);
        for (i = 1; i <= total_fleets; i++)
        {
            flt = data->getActiveFleet(i);
            for (int j = 0; j < flt->getDiscardCount(); j++)
            {
                line.clear();
                str_lst = flt->getDiscard(j);
                chars += d_file->write_vector(str_lst, 5, flt->getName());
            }
        }
        line.clear();
        if (num == 0)
            line = QString ("#_");
        line.append(QString ("-9999 0 0 0.0 0.0 # terminator for discard data "));
        chars += d_file->writeline (line);
        chars += d_file->writeline ("#");

        // mean body weight
        num = 0;
        for (i = 1; i <= total_fleets; i++) // for (i = 0; i < data->num_fleets(); i++)
            num += data->getActiveFleet(i)->getMbwtNumObs();
        temp_int = num > 0? 1: 0;
        chars += d_file->write_val(temp_int, 1, QString("use meanbodysize_data (0/1)"));
        if (temp_int == 0)
        {
            line = QString("#_COND_0 ");
        }
        else
        {
            temp_int = data->getFleet(0)->getMbwtDF();
            line = QString (QString("%1 ").arg(QString::number(temp_int)));
        }
        line.append("#_DF_for_meanbodysize_T-distribution_like");
        chars += d_file->writeline (line);
        line = QString ("# note:  type=1 for mean length; type=2 for mean body weight ");
        chars += d_file->writeline (line);
        line = QString ("#_yr month fleet part type obs stderr");
        chars += d_file->writeline (line);
        for (i = 1; i <= total_fleets; i++)
        {
            flt = data->getActiveFleet(i);
            num_lines = flt->getMbwtNumObs();
            for (int j = 0; j < num_lines; j++)
            {
                line.clear();
                str_lst = flt->getMbwtObservation(j);
                chars += d_file->write_vector(str_lst, 4);
            }
        }
        line.clear();
        if (num == 0)
            line.append("# ");
        line.append(QString("-9999 0 0 0 0 0 0 # terminator for mean body size data "));
        chars += d_file->writeline (line);
        chars += d_file->writeline ("#");


        // pop length bins
        ss_growth *grow = data->getPopulation()->Grow();
        line = QString ("# set up population length bin structure (note - irrelevant if not using size data and using empirical wtatage");
        chars += d_file->writeline (line);
        temp_int = grow->getGrowthBinMethod();
        chars += d_file->write_val(temp_int, 1, QString("length bin method: 1=use databins; 2=generate from binwidth,min,max below; 3=read vector"));
        switch (temp_int)
        {
        case 1:
            line = QString("# no additional input required.");
            chars += d_file->writeline (line);
            break;
        case 2:
            chars += d_file->write_val(grow->getGrowthBinWidth(), 1,
                        QString("binwidth for population size comp "));
            chars += d_file->write_val(grow->getGrowthBinMin(), 1,
                        QString("minimum size in the population (lower edge of first bin and size at age 0.00) "));
            chars += d_file->write_val(grow->getGrowthBinMax(), 1,
                        QString("maximum size in the population (lower edge of last bin) "));
            break;
        case 3:
            chars += d_file->write_val(grow->getNumGrowthBins() , 1,
                        QString("number of population size bins "));
            line.clear();
            chars += d_file->write_vector(grow->getGrowthBins(), 2);
            break;
        }

        // length composition
        {
        compositionLength *l_data = data->get_length_composition();

        temp_int = data->getUseLengthComp();
        chars += d_file->write_val(temp_int, 1,
                   QString("use length composition data (0/1)"));
        if (temp_int == 1)
        {
            line = QString("#_mintailcomp: upper and lower distribution for females and males separately are accumulated until exceeding this level.");
            chars += d_file->writeline (line);
            line = QString("#_addtocomp:  after accumulation of tails; this value added to all bins");
            chars += d_file->writeline (line);
            line = QString("#_combM+F: males and females treated as combined gender below this bin number ");
            chars += d_file->writeline (line);
            line = QString("#_compressbins: accumulate upper tail by this number of bins; acts simultaneous with mintailcomp; set=0 for no forced accumulation");
            chars += d_file->writeline (line);
            line = QString("#_Comp_Error:  0=multinomial, 1=dirichlet using Theta*n, 2=dirichlet using beta, 3=MV_Tweedie");
            chars += d_file->writeline (line);
            line = QString("#_ParmSelect:  consecutive index for dirichlet or MV_Tweedie");
            chars += d_file->writeline (line);
            line = QString("#_minsamplesize: minimum sample size; set to 1 to match 3.24, minimum value is 0.001");
            chars += d_file->writeline (line);
            chars += d_file->writeline("#");
            line = QString("#_mintailcomp addtocomp combM+F CompressBins CompError ParmSelect minsamplesize");
            chars += d_file->writeline (line);
            for (i = 1; i <= total_fleets; i++) // for (i = 0; i < data->num_fleets(); i++)
            {
                flt = data->getActiveFleet(i);
                line.clear();
                line.append(QString("%1 ").arg((flt->getLengthMinTailComp())));
                line.append(QString("%1 ").arg((flt->getLengthAddToData())));
                line.append(QString("%1 ").arg(QString::number(flt->getLengthCombineGen())));
                line.append(QString("%1 ").arg(QString::number(flt->getLengthCompressBins())));
                line.append(QString("%1 ").arg(QString::number(flt->getLengthCompError())));
                line.append(QString("%1 ").arg(QString::number(flt->getLengthCompErrorParm())));
                line.append(QString("%1 ").arg(QString::number(flt->getLengthMinSampleSize())));
                line.append(QString("#_fleet:%1_%2").arg(QString::number(i), flt->getName()));
                chars += d_file->writeline (line);
            }


            line = QString("# sex codes:  0=combined; 1=use female only; 2=use male only; 3=use both as joint sexxlength distribution");
            chars += d_file->writeline (line);
            line = QString("# partition codes:  (0=combined; 1=discard; 2=retained");
            chars += d_file->writeline (line);
            temp_int = l_data->getNumberBins();
            chars += d_file->write_val(temp_int, 1,
                    QString("N_LengthBins; then enter lower edge of each length bin"));
            line.clear();
            str_lst = l_data->getBinsModel()->getRowData(0);
            chars += d_file->write_vector(str_lst, 2);

            line = QString ("#_yr month fleet sex part Nsamp datavector(female-male)");
            chars += d_file->writeline (line);
            for (i = 1; i <= total_fleets; i++)
            {
                flt = data->getActiveFleet(i);
                num = flt->getLengthNumObs();
                for( int j = 0; j < num; j++)
                {
                    str_lst = flt->getLengthObservation(j);
                    chars += d_file->write_vector(str_lst, 4);
                }
            }
            line = QString("-9999");
            for (int j = 1; j < 6; j++)
                line.append(QString(" 0"));
            for (int j = 0; j < temp_int; j++)
                line.append(QString(" 0"));
            if (data->get_num_genders() > 1)
                for (int j = 0; j < temp_int; j++)
                    line.append(QString(" 0"));
            line.append(' ');
            chars += d_file->writeline (line);
            chars += d_file->writeline ("#");
            }
        }

        // age composition
        {
        compositionAge *a_data = data->get_age_composition();
        temp_int = a_data->getNumberBins();
        chars += d_file->write_val(temp_int, 1, QString("N_age_bins"));
        line.clear();
        if (temp_int > 0) {
            str_lst = a_data->getBinsModel()->getRowData(0);
            chars += d_file->write_vector(str_lst, 2);
            temp_int = a_data->number_error_defs();
            chars += d_file->write_val(temp_int, 1,
                    QString("N_ageerror_definitions"));
            for (i = 0; i < temp_int; i++)
            {
                line.clear();
                str_lst = a_data->get_error_ages(i);
                for (int j = 0; j < str_lst.count(); j++)
                    line.append(QString(" %1").arg(str_lst.at(j)));

                chars += d_file->writeline (line);
                line.clear();
                str_lst = a_data->get_error_def(i);
                for (int j = 0; j < str_lst.count(); j++)
                    line.append(QString(" %1").arg(str_lst.at(j)));

                chars += d_file->writeline (line);
            }
            line = QString("#_mintailcomp: upper and lower distribution for females and males separately are accumulated until exceeding this level.");
            chars += d_file->writeline (line);
            line = QString("#_addtocomp:  after accumulation of tails; this value added to all bins");
            chars += d_file->writeline (line);
            line = QString("#_combM+F: males and females treated as combined gender below this bin number ");
            chars += d_file->writeline (line);
            line = QString("#_compressbins: accumulate upper tail by this number of bins; acts simultaneous with mintailcomp; set=0 for no forced accumulation");
            chars += d_file->writeline (line);
            line = QString("#_Comp_Error:  0=multinomial, 1=dirichlet using Theta*n, 2=dirichlet using beta, 3=MV_Tweedie");
            chars += d_file->writeline (line);
            line = QString("#_ParmSelect:  consecutive index for dirichlet or MV_Tweedie");
            chars += d_file->writeline (line);
            line = QString("#_minsamplesize: minimum sample size; set to 1 to match 3.24, minimum value is 0.001");
            chars += d_file->writeline (line);
            chars += d_file->writeline ("#");
            line = QString("#_mintailcomp addtocomp combM+F CompressBins CompError ParmSelect minsamplesize");
            chars += d_file->writeline (line);
            for (i = 1; i <= total_fleets; i++)
            {
                flt = data->getActiveFleet(i);
                line = QString(QString("%1 %2 %3 %4 %5 %6 %7 #_fleet:%8_%9").arg (
                                   flt->getAgeMinTailComp(),
                                   flt->getAgeAddToData(),
                                   QString::number(flt->getAgeCombineGen()),
                                   QString::number(flt->getAgeCompressBins()),
                                   QString::number(flt->getAgeCompError()),
                                   QString::number(flt->getAgeCompErrorParm()),
                                   QString::number(flt->getAgeCompMinSampleSize()),
                                   QString::number(i),
                                   flt->getName()));
                chars += d_file->writeline (line);
            }
            chars += d_file->write_val(a_data->getAltBinMethod(), 1,
                    QString("Lbin_method_for_Age_Data: 1=poplenbins; 2=datalenbins; 3=lengths"));

            temp_int = 0;
            for (i = 1; i <= total_fleets; i++)
            {
                temp_int += data->getActiveFleet(i)->getAgeNumObs();
            }

            line = QString ("# sex codes:  0=combined; 1=use female only; 2=use male only; 3=use both as joint sexxlength distribution");
            chars += d_file->writeline (line);
            line = QString ("# partition codes:  (0=combined; 1=discard; 2=retained");
            chars += d_file->writeline (line);
            line = QString ("#_yr month fleet sex part ageerr Lbin_lo Lbin_hi Nsamp datavector(female-male)");
            chars += d_file->writeline (line);
            for (i = 1; i <= total_fleets; i++)
            {
                flt = data->getActiveFleet(i);
                {
                    temp_int = flt->getAgeNumObs();
                    for( int j = 0; j < temp_int; j++)
                    {
                        str_lst = flt->getAgeObservation(j);
                        chars += d_file->write_vector(str_lst, 4);
                    }
                }
            }
            line.clear();
            temp_int = a_data->getNumberBins();
            line.append("-9999 ");
            for (i = 1; i < 9; i++)
                    line.append(" 0");
            for (int j = 0; j < temp_int; j++)
                line.append(QString(" 0"));
            if (data->get_num_genders() > 1)
                for (int j = 0; j < temp_int; j++)
                    line.append(QString(" 0"));
            chars += d_file->writeline(line);
            chars += d_file->writeline ("#");
        }
        else {
            chars += d_file->writeline("#");
        }
        }

        // mean size at age
        num = 0;
        for (i = 1; i <= total_fleets; i++)
        {
            num += data->getActiveFleet(i)->getSaaModel()->rowCount();
        }
        temp_int = num > 0? 1: 0;
        chars += d_file->write_val(temp_int, 1, QString("Use_MeanSize-at-Age_obs (0/1)"));

        if (num > 0)
        {
            line = QString ("# sex codes:  0=combined; 1=use female only; 2=use male only; 3=use both as joint sexxlength distribution");
            chars += d_file->writeline (line);
            line = QString ("# partition codes:  (0=combined; 1=discard; 2=retained");
            chars += d_file->writeline (line);
            line = QString ("# ageerr codes:  positive means mean length-at-age; negative means mean bodywt_at_age");
            chars += d_file->writeline (line);
            line = QString ("#_yr month fleet sex part ageerr ignore datavector(female-male)");
            chars += d_file->writeline (line);
            line = QString ("#                                          samplesize(female-male)");
            chars += d_file->writeline (line);
            for (i = 1; i <= total_fleets; i++)
            {
                flt = data->getActiveFleet(i);
                {
                    for (int j = 0; j < flt->getSaaNumObs(); j++)
                    {
                        line.clear();
                        str_lst = flt->getSaaModel()->getRowData(j);
                        chars += d_file->write_vector(str_lst, 4);
                    }
                }
            }
            line.clear();
            line = QString("-9999 ");
            for (i = 1; i < str_lst.count(); i++)
                line.append(" 0 ");
            chars += d_file->writeline(line);
        }
        chars += d_file->writeline ("#");


        // environment observations
        temp_int = data->getNumEnvironVars();

        chars += d_file->write_val(temp_int, 1, QString("N_environ_variables"));
        line = QString ("# -2 in yr will subtract mean for that env_var; -1 will subtract mean and divide by stddev (e.g. Z-score)");
        chars += d_file->writeline(line);
        line = QString ("#Yr Variable Value");
        chars += d_file->writeline (line);
        if (temp_int > 0)
        {
            num = data->getNumEnvironVarObs();
            for (i = 0; i < num; i++)
            {
                line.clear();
                str_lst = data->get_environ_var_obs(i);
                for (int j = 0; j < str_lst.count(); j++)
                    line.append(QString(" %1").arg(str_lst.at(j)));
                chars += d_file->writeline (line);
            }
            line = QString("-9999  0  0  # terminator");
            chars += d_file->writeline(line);
        }
        chars += d_file->writeline ("#");

        // general composition methods
        line = QString ("# Sizefreq data. Defined by method because a fleet can use multiple methods");
        chars += d_file->writeline (line);
        num = data->getNumGeneralCompMethods();
        chars += d_file->write_val(num, 1, QString("N sizefreq methods to read (or -1 for expanded options)"));
        if (num > 0)
        {
            line.clear();
            for (i = 0; i < num; i++)
            {
                temp_str = QString::number(data->getGeneralCompMethod(i)->getNumberBins());
                line.append(QString("%1 ").arg(temp_str));
            }
            line.append("# Sizefreq N bins per method");
            chars += d_file->writeline (line);
            line.clear();
            for (i = 0; i < num; i++)
            {
                temp_str = QString::number(data->getGeneralCompMethod(i)->getUnits());
                line.append(QString("%1 ").arg(temp_str));
            }
            line.append("# Sizetfreq units (1=bio/2=num) per method");
            chars += d_file->writeline (line);
            line.clear();
            for (i = 0; i < num; i++)
            {
                temp_str = QString::number(data->getGeneralCompMethod(i)->getScale());
                line.append(QString("%1 ").arg(temp_str));
            }
            line.append("# Sizefreq scale (1=kg/2=lbs/3=cm/4=inches) per method");
            chars += d_file->writeline (line);
            line.clear();
            for (i = 0; i < num; i++)
            {
                temp_str = QString::number(data->getGeneralCompMethod(i)->getMinComp());
                line.append(QString("%1 ").arg(temp_str));
            }
            line.append("# Sizefreq mincomp per method ");
            chars += d_file->writeline (line);
            line.clear();
            for (i = 0; i < num; i++)
            {
                temp_int = 0;
                for (j = 1; j <= total_fleets; j++)
                {
                    flt = data->getActiveFleet(j);
                    {
                        temp_int += flt->getGenNumObs(i);
                    }
                }
                line.append(QString("%1 ").arg(QString::number(temp_int)));
            }
            line.append("# Sizefreq N obs per method");
            chars += d_file->writeline (line);
            line.clear();
            for (i = 0; i < num; i++)
            {
                temp_str = QString::number(data->getGeneralCompMethod(i)->getCompErrorType());
                line.append(QString("%1 ").arg(temp_str));
            }
            line.append("# Comp_Error: 0=multinomial; 1=dirichlet using Theta*n; 2=dirichelt using beta; 3=MV_Tweedie");
            chars += d_file->writeline (line);
            line.clear();
            for (i = 0; i < num; i++)
            {
                temp_str = QString::number(data->getGeneralCompMethod(i)->getCompErrorIndex());
                line.append(QString("%1 ").arg(temp_str));
            }
            line.append("# Comp_Error: index for dirchelt or MV_Tweedie");
            chars += d_file->writeline (line);
            line = QString("#_Sizefreq bins");
            chars += d_file->writeline (line);
            for (i = 0; i < num; i++)
            {
                line.clear();
                str_lst = data->getGeneralCompMethod(i)->getBinsModel()->getRowData(0);
                for (int j = 0; j < str_lst.count(); j++)
                    line.append(QString (" %1").arg (str_lst.at(j)));

                chars += d_file->writeline (line);
            }
            line = QString ("# Method Yr Month Flt Gender Part Nsamp datavector(female-male)");
            chars += d_file->writeline (line);
            for (i = 0; i < num; i++)
            {
                for (j = 1; j <= total_fleets; j++) // for (int j = 0; j < data->num_fleets(); j++)
                {
                    flt = data->getActiveFleet(j);
                    {
                        num_lines = flt->getGenNumObs(i);
                        for (int k = 0; k < num_lines; k++)
                        {
                            line.clear();
                            str_lst = flt->getGenObservation(i, k);
                            chars += d_file->write_vector(str_lst, 4);
                        }
                    }
                }
            }
        }
        chars += d_file->writeline ("#");

        // tag recapture
        temp_int = data->getDoTags()? 1: 0;
        chars += d_file->write_val(temp_int, 1, QString("do tags (0/1/2); where 2 allows entry of TG_min_recap"));
        if (temp_int == 1)
        {
            num = data->getNumTagGroups();
            chars += d_file->write_val(num, 1, QString("N tag groups"));
            temp_int = 0;
            for (i = 1; i <= total_fleets; i++) // for (i = 0; i < data->num_fleets(); i++)
            {
                    temp_int += data->getActiveFleet(i)->getRecapNumEvents();
            }
            chars += d_file->write_val(temp_int, 1, QString("N recap events"));
            temp_int = data->getTagLatency();
            chars += d_file->write_val(temp_int, 1, QString("mixing latency period: N periods to delay before comparing observed to expected recoveries (0 = release period)"));
            temp_int = data->getTagMaxPeriods();
            chars += d_file->write_val(temp_int, 1, QString("max periods (seasons) to track recoveries, after which tags enter accumulator"));
            line = QString (QString("#_Release data for each tag group.  Tags are considered to be released at the beginning of a season (period)"));
            chars += d_file->writeline (line);
            line = QString (QString("#<TG> area yr season <tfill> gender age Nrelease  (note that the TG and tfill values are placeholders and are replaced by program generated values"));
            chars += d_file->writeline (line);
            for (i = 0; i < num; i++)
            {
                line.clear();
                str_lst = data->getTagObservation(i);
                chars += d_file->write_vector(str_lst, 4);
            }
            line = QString("# Recapture data");
            chars += d_file->writeline (line);
            line = QString("# TAG  Yr Season Fleet Nrecap");
            chars += d_file->writeline (line);
            for (i = 1; i <= num; i++)
            {
                for (j = 1; j <= total_fleets; j++) // for (int j = 0; j < data->num_fleets(); j++)
                {
                    flt = data->getActiveFleet(j);
                    {
                        num_lines = flt->getRecapNumEvents();
                        for (int k = 0; k < num_lines; k++)
                        {
                            line.clear();
                            str_lst = flt->getRecapObservation(k);
                            if (str_lst.at(0).toInt() == i)
                            {
                                chars += d_file->write_vector(str_lst, 4);
                            }
                        }
                    }
                }
            }
        }
        chars += d_file->writeline(QString("#"));

        // morph composition
        temp_int = data->getDoMorphComp()? 1: 0;
        chars += d_file->write_val(temp_int, 1, QString("morphcomp data(0/1)"));
        line = QString ("# Nobs, Nmorphs, mincomp");
        chars += d_file->writeline (line);
        line = QString ("# yr, seas, type, partition, Nsamp, datavector_by_Nmorphs");
        chars += d_file->writeline (line);
        if (temp_int == 1)
        {
            num_lines = 0;
            for (i = 0; i < data->get_num_fleets(); i++)
                num_lines += data->getFleet(i)->getMorphNumObs();
            line = QString (QString("%1 # N_observations").arg(QString::number(num_lines)));
            chars += d_file->writeline (line);
            temp_int = data->get_morph_composition()->getNumberMorphs();
            line = QString (QString("%1 # N_morphs").arg(QString::number(temp_int)));
            chars += d_file->writeline (line);
            temp_str = data->getFleet(0)->getMorphMinTailComp();//get_morph_composition()->mincomp();
            line = QString (QString("%1 # Mincomp").arg(temp_str));//QString::number(temp_float)));
            chars += d_file->writeline (line);
            temp_int = data->getTagLatency();
            line = QString("# Year month fleet partition Nsamp data_vector");
            chars += d_file->writeline (line);
            for (i = 1; i <= total_fleets; i++) // for (num = 0; num < data->num_fleets(); num++)
            {
                flt = data->getActiveFleet(i);
                {
                    num_lines = flt->getMorphNumObs();
                    for (int j = 0; j < num_lines; j++)
                    {
                        line.clear();
                        str_lst = flt->getMorphObservation(j);
                        chars += d_file->write_vector(str_lst, 4);
                    }
                }
            }
        }
        chars += d_file->writeline ("#");

        temp_int = data->getReadSelectivityPriors()? 1: 0;
        chars += d_file->write_val(temp_int, 1, QString("Do dataread for selectivity priors(0/1)"));
        line = QString("# Yr, seas, Fleet,  Age/Size,  Bin,  selex_prior,  prior_sd");
        chars += d_file->writeline (line);
        line = QString("# feature not yet implemented");
        chars += d_file->writeline (line);
        chars += d_file->writeline ("#");

        //end of data
        chars += d_file->write_val(END_OF_DATA);
        d_file->newline();
        chars += d_file->writeline ("ENDDATA");


        d_file->close();
    }
    return chars;
}

bool read33_forecastFile(ss_file *f_file, ss_model *data)
{
    QString token;
    QString temp_str;
    bool temp_bool;
    QStringList str_lst(" ");
    float temp_float;
    int temp_int = 0;
    int i, num, fleet, area;
    int numFleets = 1, numSeas = 1, numGenders = 1;

    if(f_file->open(QIODevice::ReadOnly))
    {
        //  SS_Label_Info_3.0 #Read forecast.ss
        ss_forecast *fcast = data->forecast;
        fcast->reset();
        f_file->seek(0);
        f_file->resetLineNum();
        f_file->setOkay(true);
        f_file->setStop(false);
        f_file->read_comments();

        numFleets = data->get_num_fleets();
        numSeas  = data->get_num_seasons();
        numGenders = (data->get_num_genders() > 1)? 2: 1;

        fcast->set_num_seasons(numSeas);
        fcast->set_num_fleets(numFleets);
        fcast->set_num_genders(numGenders);

        if (f_file->getOkay() && !f_file->getStop()) {
        temp_int = f_file->getIntValue(QString("Benchmarks/Reference points"), 0, 3, 1);
        fcast->set_benchmarks(temp_int);
        }
        if (f_file->getOkay() && !f_file->getStop()) {
        temp_int = f_file->getIntValue(QString("MSY Method"), 1, 5, 1);
        fcast->set_MSY(temp_int);
        if (temp_int == 5)
        {
            temp_int = f_file->getIntValue(QString("MSY Units"), 1, 4, 1);
            fcast->setMsyUnits(temp_int);
            // read fleet, cost/F, price/mt until fleet==-9999
            do {
                str_lst.clear();
                token = f_file->get_next_value(QString("fleet"));
                temp_int = token.toInt();
                str_lst.append(token);
                token = f_file->get_next_value(QString("cost/F"));
                str_lst.append(token);
                token = f_file->get_next_value(QString("price/mt"));
                str_lst.append(token);
                token = f_file->get_next_value(QString("include"));
                str_lst.append(token);
                if (temp_int == -999)
                    break;
                fcast->appendMsyCosts(str_lst);
            } while (temp_int != -999);
        }
        }
        if (f_file->getOkay() && !f_file->getStop()) {
        token = f_file->get_next_value(QString("SPR target"));
        temp_float = token.toFloat();
        fcast->set_spr_target(temp_float);
        }
        if (f_file->getOkay() && !f_file->getStop()) {
        token = f_file->get_next_value(QString("biomass target"));
        temp_float = token.toFloat();
        fcast->set_biomass_target(temp_float);
        }
        if (f_file->getOkay() && !f_file->getStop()) {
        if (fcast->get_benchmarks() == 3)
        {
            token = f_file->get_next_value(QString("F at Blimit"));
            fcast->set_blimit(token.toDouble());
        }
        else
        {
            fcast->set_blimit(-0.25);
        }
        }
        if (f_file->getOkay() && !f_file->getStop()) {
        for (i = 0; i < 10; i++)
        {
            token = f_file->get_next_value(QString("benchmark year"));
            temp_int = token.toInt();
            fcast->set_benchmark_years(i, temp_int);
        }
        temp_int = f_file->getIntValue(QString("Benchmark Rel F basis"), 0, 2, 1);
        fcast->set_benchmark_rel_f(temp_int);
        }

        if (f_file->getOkay() && !f_file->getStop()) {
        temp_int = f_file->getIntValue(QString("Forecast type"), -1, 5, 2);
        fcast->set_forecast(temp_int);
        token = f_file->get_next_value(QString("number of forecast years"));
        temp_int = token.toInt();
        fcast->set_num_forecast_years(temp_int);
        token = f_file->get_next_value(QString("F mult"));
        temp_float = token.toFloat();
        fcast->set_f_mult(temp_float);
        }
        if (f_file->getOkay() && !f_file->getStop()) {
        for (i = 0; i < 6; i++)
        {
            token = f_file->get_next_value(QString("forecast year"));
            temp_int = token.toInt();
            fcast->set_forecast_year(i, temp_int);
        }
        }

        if (f_file->getOkay() && !f_file->getStop()) {
        temp_int = f_file->getIntValue(QString("Forecast selectivity"), 0, 1, 0);
        fcast->setSelectivity (temp_int);
        temp_int = f_file->getIntValue(QString("Control Rule"), 0, 4, 1);
        fcast->set_cr_method(temp_int);
        token = f_file->get_next_value(QString("control rule upper limit"));
        temp_float = token.toFloat();
        fcast->set_cr_biomass_const_f(temp_float);
        token = f_file->get_next_value(QString("control rule lower limit"));
        temp_float = token.toFloat();
        fcast->set_cr_biomass_no_f(temp_float);
        token = f_file->get_next_value(QString("control rule buffer"));
        temp_float = token.toFloat();
        fcast->set_cr_buffer(temp_float);
        }
        if (f_file->getOkay() && !f_file->getStop()) {
        temp_int = token.toInt();
        if (temp_int == -1) {
            do {
                str_lst.clear();
                token = f_file->get_next_value(QString("control rule buffer year"));
                temp_int = token.toInt();
                str_lst.append(token);
                token = f_file->get_next_value(QString("control rule buffer value"));
                str_lst.append(token);
                if (temp_int == END_OF_LIST)
                    break;
                fcast->append_cr_buffer_list(str_lst);
            } while (temp_int != END_OF_LIST);
        }
        }
        if (f_file->getOkay() && !f_file->getStop()) {
        temp_int = f_file->getIntValue(QString("Number of forecast loops"), 1, 3, 3);
        fcast->set_num_forecast_loops(temp_int);
        temp_int = f_file->getIntValue(QString("First forecast loop with stochastic recruitment"), 1, 3, 3);
        fcast->set_forecast_loop_first(temp_int);
        temp_int = f_file->getIntValue(QString("Forecast recruitment"), 0, 5, 0);
        fcast->set_forecast_recr_adjust(temp_int);
        token = f_file->get_next_value(QString("Scalar or N years to ave recruitment"));
        temp_float = token.toFloat();
        fcast->set_forecast_recr_adj_value(temp_float);
        token = f_file->get_next_value(QString("MGparm averaging"));
        temp_int = token.toInt();
        fcast->setMGparmAveraging(temp_int);
        if (temp_int == 1)
        {
            num = 0;
            do {
                str_lst.clear();
                str_lst.append(f_file->get_next_value(QString("Type")));
                str_lst.append(f_file->get_next_value(QString("Method")));
                str_lst.append(f_file->get_next_value(QString("Min year")));
                str_lst.append(f_file->get_next_value(QString("Max year")));
                temp_int = str_lst[0].toInt();
                if (temp_int == END_OF_LIST)
                    break;
                fcast->setMGparmAveLine(num, str_lst);
                num++;
            } while (temp_int != END_OF_LIST);
        }
        }
        if (f_file->getOkay() && !f_file->getStop()) {
        token = f_file->get_next_value(QString("caps and allocs first yr"));
        temp_int = token.toInt();
        fcast->set_caps_alloc_st_year(temp_int);
        }
        if (f_file->getOkay() && !f_file->getStop()) {
        token = f_file->get_next_value(QString("std dev log(catch/tgt)"));
        temp_float = token.toFloat();
        fcast->set_log_catch_std_dev(temp_float);
        }
        if (f_file->getOkay() && !f_file->getStop()) {
        temp_int = f_file->getIntValue(QString("Do West Coast rebuilder"), 0, 2, 0);
        fcast->set_do_rebuilder(temp_int);
        token = f_file->get_next_value(QString("rebuilder: first year"));
        temp_int = token.toInt();
        fcast->set_rebuilder_first_year(temp_int);
        token = f_file->get_next_value(QString("rebuilder: curr year"));
        temp_int = token.toInt();
        fcast->set_rebuilder_curr_year(temp_int);
        }
        if (f_file->getOkay() && !f_file->getStop()) {
        temp_int = f_file->getIntValue(QString("Fleet Relative F"), 1, 2, 1);
        fcast->set_fleet_rel_f(temp_int);

        temp_int = f_file->getIntValue(QString("Basis for max forecast catch"), 2, 6, 2);
        fcast->set_catch_tuning_basis(temp_int);
        }
        // season, fleet, relF
        if (f_file->getOkay() && !f_file->getStop()) {
        if (fcast->get_fleet_rel_f() == 2)
        {
            for (i = 0; i < numSeas; i++)
                for (int j = 0; j < numFleets; j++)
                    fcast->setSeasFleetRelF(i, j, 1.0);
            do {
                temp_int = f_file->getIntValue(QString("Season"), 1, numSeas, 1);
                fleet = f_file->getIntValue(QString("Fleet num"), 1, numFleets, 1);
                temp_float = f_file->get_next_value(QString("relF")).toFloat();
                if (temp_int != END_OF_LIST)
                    fcast->setSeasFleetRelF(temp_int, fleet, temp_float);

            } while (temp_int != END_OF_LIST);
        }
        }
        // max catch fleet
        if (f_file->getOkay() && !f_file->getStop()) {
        do {
            fleet = f_file->get_next_value().toInt();
            temp_float = f_file->get_next_value(QString("max catch fleet")).toFloat();
            if (fleet != END_OF_LIST)
                fcast->set_max_catch_fleet((fleet - 1), temp_float);
        } while (fleet != END_OF_LIST);
        }
        // max catch area
        if (f_file->getOkay() && !f_file->getStop()) {
        for (i = 0; i < fcast->get_num_areas(); i++)
            fcast->set_max_catch_area(i, 0);
        do {
            area = f_file->get_next_value().toInt();
            temp_float = f_file->get_next_value(QString("max catch area")).toFloat();
            if (area != END_OF_LIST)
                fcast->set_max_catch_area((area - 1), temp_float);
        } while (area != END_OF_LIST);
        }
        // Allocation groups
        if (f_file->getOkay() && !f_file->getStop()) {
        fcast->set_num_alloc_groups(0);
        for (i = 0; i < data->get_num_fleets(); i++)
            data->getFleet(i)->setAllocGroup(0);
        do {
            fleet = f_file->get_next_value(QString("fleet alloc grp")).toInt();
            temp_int = f_file->get_next_value(QString("alloc grp")).toInt();
            if (fleet != END_OF_LIST)
            {
                data->getFleet(fleet - 1)->setAllocGroup(temp_int);
                fcast->setAllocGrp(fleet - 1, temp_int);
            }
        } while (fleet != END_OF_LIST);
        }
        if (f_file->getOkay() && !f_file->getStop()) {
        if (fcast->get_num_alloc_groups() > 0)
        {
            fcast->reset_alloc_fractions();
            int j = 0;
            do
            {
                str_lst.clear();
                token = f_file->get_next_value(QString("alloc grp frac year"));
                temp_int = token.toInt();
                str_lst.append(token);
                for (i = 0; i < fcast->get_num_alloc_groups(); i++)
                {
                    token = f_file->get_next_value(QString("alloc group fraction"));
                    str_lst.append(token);
                }
                if (temp_int != END_OF_LIST)
                    fcast->setAllocFractions(j++, str_lst);
            } while (temp_int != END_OF_LIST);
        }
        }
        // Forecast Catch
        if (f_file->getOkay() && !f_file->getStop()) {
        fcast->setNumFixedFcastCatch(0);
        token = f_file->get_next_value(QString("Basis for forecast catch"));
        num = token.toInt();
        fcast->set_input_catch_basis(num);
        do
        {
            str_lst.clear();
            token = f_file->get_next_value(QString("Year"));
            temp_int = token.toInt();
            str_lst.append(token);                // Year
            str_lst.append(f_file->get_next_value(QString("Season"))); // Season
            str_lst.append(f_file->get_next_value(QString("Fleet"))); // Fleet
            str_lst.append(f_file->get_next_value(QString("Catch"))); // Catch
            if (num == -1)
                str_lst.append(f_file->get_next_value(QString("Basis"))); // Basis
            if (temp_int != END_OF_LIST)
                fcast->addFixedFcastCatch(str_lst);
        } while (temp_int != END_OF_LIST);
        }
        //  SS_Label_Info_3.5 #End of datafile indicator
        if (f_file->getOkay() && !f_file->getStop()) {
        token = f_file->get_next_value(QString("End of data indicator"));
        temp_int = token.toInt();
        if (temp_int != END_OF_DATA)
        {
            if (f_file->atEnd())
            {
                f_file->error(QString("Must have 999 to verify end of forecast inputs!"));
            }
            else
            {
                if (fcast->get_forecast() > 0)
                    f_file->error(QString("Stopped reading before end of data marker!"));
            }
        }
        }
        f_file->close();
    }
    else
    {
        f_file->error(QString("Forecast file does not exist or is not readable."));
    }
    return 1;
}

int write33_forecastFile(ss_file *f_file, ss_model *data)
{
    int temp_int, num, i, chars = 0;
    int yr = 0, bmarks = 0, msy = 0;
    float temp_float;
    bool temp_bool;
    QString value, line, temp_string;
    QStringList str_lst, tmp_lst;
    ss_forecast *fcast = data->forecast;


    if(f_file->open(QIODevice::WriteOnly))
    {
        chars += writeVersionComment(f_file);
        chars += f_file->write_comments();

        line = QString("# for all year entries except rebuilder; enter either: actual year, -999 for styr, 0 for endyr, neg number for rel. endyr");
        chars += f_file->writeline(line);

        bmarks = fcast->get_benchmarks();
        chars += f_file->write_val(bmarks, 1,
                    QString("Benchmarks: 0=skip; 1=calc F_spr,F_btgt,F_msy; 2=calc F_spr,F_0.1,F_msy; 3=add F_Blimit"));

        msy = fcast->get_MSY();
        chars += f_file->write_val(msy, 1,
                    QString("Do_MSY: 1=set to F(SPR); 2=calc F(MSY); 3=set to F(Btgt) or F0.1; 4=set to F(endyr); 5=calc F(MEY) with MSY_unit options"));
        chars += f_file->writeline("# if Do_MSY=5, enter MSY_Units; then list fleet_ID, cost/F, price/mt, include_in_Fmey_scaling");
        chars += f_file->writeline("#   use -fleet_ID to fill; -999 to terminate");
        if (msy == 5)
        {
            // write msyUnits
            temp_int = fcast->getMsyUnits();
            chars += f_file->write_val(temp_int, 1,
                        QString("MSY_units: 1=dead biomass, 2=retained biomass, 3=profits"));
            chars += f_file->writeline("# Fleet Cost_per_F Price_per_F include_in_Fmey_search");
            temp_int = fcast->getMsyCosts()->rowCount();
            for (i = 0; i < temp_int; i++)
            {
                f_file->write_vector(fcast->getMsyCostRow(i), 2);
            }
            f_file->writeline("-999 1 1 1 # terminate list of fleet costs and prices");
        }

        chars += f_file->write_val(fcast->get_spr_target(), 1,
                    QString("SPR target (e.g. 0.40)"));

        chars += f_file->write_val(fcast->get_biomass_target(), 1,
                    QString("Biomass target (e.g. 0.40)"));
        if (bmarks == 3)
        {
            chars += f_file->write_val(fcast->get_blimit(), 1, QString("COND: Do_Benchmark=3: Blimit as fraction of Bmsy (neg value to use as frac of Bzero) (e.g. 0.50)"));
        }

        line = QString("#_Bmark_years: beg_bio, end_bio, beg_selex, end_selex, beg_relF, end_relF, beg_recr_dist, end_recr_dist, beg_SRparm, end_SRparm");
        line.append(QString(" (enter actual year, or values of 0 or -integer to be rel. endyr)"));
        chars += f_file->writeline(line);
        line.clear();
        temp_string = QString("# ");
        for (i = 0; i < 10; i++)
        {
            temp_int = fcast->get_benchmark_year(i);
            value = QString::number(temp_int);
            line.append(QString(QString(" %1").arg(value)));
            temp_string.append(QString(" %1").arg(QString::number(data->refyearvalue(temp_int))));// data->get_end_year() + temp_int)));
        }
        chars += f_file->writeline(line);
        temp_string.append(" #_after processing ");
        chars += f_file->writeline(temp_string);
        chars += f_file->writeline("# value <0 convert to endyr-value; except -999 converts to start_yr; must be >=start_yr and <=endyr");

        chars += f_file->write_val(fcast->get_benchmark_rel_f(), 1,
                      QString("Bmark_relF_Basis: 1 = use year range; 2 = set relF same as forecast below"));

        chars += f_file->writeline("#");

        chars += f_file->write_val(fcast->get_forecast(), 1,
                      QString("Forecast: -1=none; 0=simple_1yr; 1=F(SPR); 2=F(MSY) 3=F(Btgt) or F0.1; 4=Ave F (uses first-last relF yrs); 5=input annual F scalar"));
        chars += f_file->writeline("# where none and simple require no input after this line; simple sets forecast F same as end year F");

        chars += f_file->write_val(fcast->get_num_forecast_years(), 1, QString("N forecast years"));

        chars += f_file->write_val(fcast->get_f_scalar(), 1, QString("Fmult (only used for Do_Forecast==5) such that apical_F(f)=Fmult*relF(f)"));

        line = QString("#_Fcast_years:  beg_selex, end_selex, beg_relF, end_relF, beg_mean recruits, end_recruits  (enter actual year, or values of 0 or -integer to be rel. endyr)");
        chars += f_file->writeline(line);
        line.clear();
        temp_string = QString("# ");
        for (int i = 0; i < 6; i++)
        {
            value = QString::number(fcast->get_forecast_year(i));
            temp_int = value.toInt();
            line.append(QString(QString(" %1").arg(value)));
            temp_string.append(QString(" %1").arg(QString::number(data->refyearvalue(temp_int))));//data->get_end_year() + fcast->get_forecast_year(i))));
        }
        chars += f_file->writeline(line);
        temp_string.append(" #_after processing ");
        chars += f_file->writeline(temp_string);

        chars += f_file->write_val(fcast->getSelectivity(), 1, QString("Forecast selectivity (0=fcast selex is mean from year range; 1=fcast selectivity from annual time-vary parms)"));
        chars += f_file->write_val(fcast->get_cr_method(), 1, QString("Control rule method (0: none; 1: ramp does catch=f(SSB), buffer on F; 2: ramp does F=f(SSB), buffer on F; 3: ramp does catch=f(SSB), buffer on catch; 4: ramp does F=f(SSB), buffer on catch) "));
        chars += f_file->writeline("# values for top, bottom and buffer exist, but not used when Policy=0");
        chars += f_file->write_val(fcast->get_cr_biomass_const_f(), 1, QString("Control rule inflection for constant F (as frac of Bzero, e.g. 0.40); must be > control rule cutoff, or set to -1 to use Bmsy/SSB_unf"));
        chars += f_file->write_val(fcast->get_cr_biomass_no_f(), 1, QString("Control rule cutoff for no F (as frac of Bzero, e.g. 0.10) "));
        temp_float = fcast->get_cr_buffer();
        chars += f_file->write_val(temp_float, 1, QString("Buffer:  enter Control rule target as fraction of Flimit (e.g. 0.75), negative value invokes list of [year, scalar] with filling from year to YrMax "));
        if (static_cast<int>(temp_float) == -1) {
            chars += f_file->writeline(QString("#_annual control rule buffer"));
            chars += f_file->writeline(QString("#_year    value"));
            for (i = 0; i < fcast->get_num_cr_buffer_rows(); i++) {
                chars += f_file->write_vector(fcast->get_cr_buffer_row(i), 2);
            }
            chars += f_file->writeTerminator(1);
        }

        chars += f_file->write_val(fcast->get_num_forecast_loops(), 1, QString("N forecast loops (1=OFL only; 2=ABC; 3=get F from forecast ABC catch with allocations applied)"));
        chars += f_file->write_val(fcast->get_forecast_loop_first(), 1, QString("First forecast loop with stochastic recruitment"));
        chars += f_file->write_val(fcast->get_forecast_recr_adjust(), 1, QString("Forecast recruitment: 0=spawn_recr; 1=value*spawn_recr_fxn; 2=value*VirginRecr; 3=recent mean from yr range above (need to set phase to -1 in control to get constant recruitment in MCMC)"));
        chars += f_file->write_val(fcast->get_forecast_recr_adj_value(), 1, QString("value is multiplier of SRR "));
        temp_bool = fcast->getMGparmAveraging();
        chars += f_file->write_val((temp_bool?1:0), 1, QString("MGparam averaging"));
        if (!temp_bool)
        {
            line = QString("# Conditional input if MGparam averaging == 1");
            chars += f_file->writeline(line);
            line = QString("# enter list of:  type, st_year, end_year and end with type=-9999");
            chars += f_file->writeline(line);
        }
        if (temp_bool)
        {
            line = QString("# type method st_year end_year ");
            chars += f_file->writeline(line);
            for (int i = 0, total = fcast->getNumMGparmAve(); i < total; i++)
            {
                chars += f_file->write_vector(fcast->getMGparmAveLine(i), 1);
            }
            chars += f_file->writeline(QString(" -9999 -1 -1 -1"));
        }

        chars += f_file->write_val(fcast->get_caps_alloc_st_year(), 1, QString("FirstYear for caps and allocations (should be after years with fixed inputs)"));

        chars += f_file->write_val(fcast->get_log_catch_std_dev(), 1, QString("stddev of log(realized catch/target catch) in forecast (set value>0.0 to cause active impl_error)"));

        chars += f_file->write_val((fcast->get_do_rebuilder()? 1: 0), 1, QString("Do West Coast gfish rebuilder output: 0=no; 1=yes "));
        chars += f_file->write_val(fcast->get_rebuilder_first_year(), 1, QString ("Rebuilder: first year catch could have been set to zero (Ydecl)(-1 to set to 1999)"));
        chars += f_file->write_val(fcast->get_rebuilder_curr_year(), 1, QString ("Rebuilder: year for current age structure (Yinit) (-1 to set to endyear+1)"));

        chars += f_file->write_val(fcast->get_fleet_rel_f(), 1, QString ("fleet relative F: 1=use first-last alloc year; 2=read seas, fleet, alloc list below"));
        line = QString("# Note that fleet allocation is used directly as average F if Do_Forecast=4");
        chars += f_file->writeline(line);

        chars += f_file->write_val(fcast->get_catch_tuning_basis(), 1, QString ("basis for fcast catch tuning and for fcast catch caps and allocation  (2=deadbio; 3=retainbio; 5=deadnum; 6=retainnum)"));

        line = QString("# Conditional input if relative F choice = 2");
        chars += f_file->writeline(line);
        line = QString("# enter list of:  season,  fleet, relF; if used, terminate with season=-9999");
        chars += f_file->writeline(line);

        if (fcast->get_fleet_rel_f() == 2)
        {
            temp_string = QString("");
            for (int seas = 0; seas < data->get_num_seasons(); seas++)
            {
                str_lst = fcast->getSeasFleetRelF(seas);
                for (i = 0; i < data->get_num_fleets(); i++)
                {
                    temp_float = QString(str_lst.at(i)).toFloat();
                    if (temp_float > 0 || temp_float < 0)
                    {
                        tmp_lst.clear();
                        tmp_lst.append(QString::number(seas + 1));
                        tmp_lst.append(QString::number(i + 1));
                        tmp_lst.append(QString::number(temp_float));
                        chars += f_file->write_vector(tmp_lst, 5);
                    }
                }
                tmp_lst.clear();
                chars += f_file->writeline(" -9999 1 0.0");
            }
        }
        else
        {
            temp_string = QString("#  -9999 1 0.0");
            chars += f_file->writeline(temp_string);
        }
        temp_string.clear();

        line = QString("# enter list of: fleet number, max annual catch for fleets with a max; terminate with fleet=-9999");
        chars += f_file->writeline(line);
        str_lst = fcast->getMaxCatchFleets();
        line.clear();
        for (i = 0; i < fcast->get_num_fleets(); i++)
        {
            line.clear();
            value = str_lst.at(i);
            if (data->getFleet(i)->isActive())
            {
                if (value.toFloat() > 0)
                {
                    line.append(QString("%1 %2").arg(QString::number(i+1), value));
                    chars += f_file->writeline(line);
                }
            }
        }
        chars += f_file->writeline(QString ("-9999 -1"));

        chars += f_file->writeline(QString("# enter list of area ID and max annual catch; terminate with area=-9999"));
        str_lst = fcast->getMaxCatchAreas();
        for (i = 0; i < fcast->get_num_areas(); i++)
        {
            line.clear();
            value = str_lst.at(i);
            temp_float = value.toFloat();
            if (temp_float > 0 || temp_float < 0)
            {
                line.append(QString("%1 %2").arg(QString::number(i+1), value));
                chars += f_file->writeline(line);
            }
        }
        chars += f_file->writeline(QString ("-9999 -1"));

        // allocation groups
        line = QString("# enter list of fleet number and allocation group assignment, if any; terminate with fleet=-9999");
        chars += f_file->writeline(line);
        str_lst = fcast->getAllocGrpList();
        line.clear();
        if (fcast->get_num_alloc_groups() > 0 && msy != 5)
        {
            for (i = 0; i < fcast->get_num_fleets(); i++)
            {
                if (data->getFleet(i)->isActive())
                {
                    line.clear();
                    value = str_lst.at(i);
                    if (value.toInt() > 0)
                    {
                        line = QString("%1 %2").arg(QString::number(i+1), value);
                        chars += f_file->writeline(line);
                    }
                }
            }
        }
        line = QString ("-9999 -1");
        chars += f_file->writeline(line);

        fcast->getAllocFractModel()->sort(0);
        temp_string = QString::number(fcast->get_num_alloc_groups());
        chars += f_file->writeline(QString("#_if N allocation groups >0, list year, allocation fraction for each group "));
        chars += f_file->writeline(QString("# list sequentially because read values fill to end of N forecast"));
        chars += f_file->writeline(QString("# terminate with -9999 in year field "));
        line.clear();
        if (fcast->get_num_alloc_groups() > 0)
        {
            line = QString(QString("#Yr alloc frac for each of: %1 alloc grps").arg(temp_string));
            chars += f_file->writeline(line);
            line.clear();
            for (yr = 0; yr < fcast->get_num_forecast_years(); yr++)
            {
                line.clear();
                str_lst = fcast->getAllocFractions(yr);
                if (!str_lst.at(0).isEmpty())
                    chars += f_file->write_vector(str_lst, 5);
/*                for (i = 0; i < str_lst.count(); i++)
                {
                    line.append(QString(QString(" %1").arg(str_lst.at(i))));
                }
                chars += f_file->writeline(line);*/
            }
            line = QString("-9999");
            for (i = 0; i < fcast->get_num_alloc_groups(); i++)
                line.append(QString(" 1"));
            chars += f_file->writeline(line);
        }
        else
        {
            chars += f_file->writeline(QString ("# no allocation groups"));
        }

        temp_int = fcast->get_input_catch_basis();
        chars += f_file->write_val(temp_int, 1, QString("basis for input Fcast catch: -1=read basis with each obs; 2=dead catch; 3=retained catch; 99=input apical_F; NOTE: bio vs num based on fleet's catchunits"));
        chars += f_file->writeline(QString("#enter list of Fcast catches or Fa; terminate with line having year=-9999"));
        num = fcast->getNumFixedFcastCatch();
        if (temp_int < 0)
        {
            chars += f_file->writeline(QString("#_Yr Seas Fleet Catch(or_F) Basis"));
            for (i = 0; i < num; i++)
            {
                str_lst = fcast->getFixedFcastCatch(i);
                if (str_lst.count() == 4)
                    str_lst.append(QString("2"));
                chars += f_file->write_vector(str_lst, 4);
            }
            chars += f_file->writeline(QString("-9999 1   1   0   2"));
        }
        else
        {
            chars += f_file->writeline(QString("#_Yr Seas Fleet Catch(or_F)"));
            for (i = 0; i < num; i++)
            {
                str_lst = fcast->getFixedFcastCatch(i);
                chars += f_file->write_vector(str_lst, 4);
            }
            chars += f_file->writeline(QString("-9999 1   1   0 "));
        }

        chars += f_file->writeline("#");

        chars += f_file->write_val(END_OF_DATA, 6, QString("verify end of input "));

        f_file->close();
    }
    return chars;
}

bool read33_controlFile(ss_file *c_file, ss_model *data)
{
    int i = 0, temp_int = 0, index = 0, num = 0, num_vals = 0;
    float temp_float = 0;
    QString temp_string;
    QString msg;
    QStringList datalist;
    population * pop = data->pPopulation;
    int flt = 0;
    int num_fleets = data->get_num_fleets();
    int timevaryread = 0;
    int num_settle_timings = 0;
//    int startYear = data->get_start_year();
//    int endYear = data->get_end_year();

    if(c_file->open(QIODevice::ReadOnly))
    {
        c_file->seek(0);
        c_file->resetLineNum();
        c_file->setOkay(true);
        c_file->setStop(false);
        c_file->read_comments();

#ifdef DEBUG_CONTROL
    QMessageBox::information(nullptr, "Information - reading contol file", "Control file start.");
#endif
        // read wtatage.ss
    if (c_file->getOkay() && !c_file->getStop()) {
        temp_int = c_file->getIntValue(QString("Read wtatage.ss?"), 0, 2, 0);
        data->setReadWtAtAge(temp_int);
    }
        // growth patterns
    if (c_file->getOkay() && !c_file->getStop()) {
        num = c_file->get_next_value(QString("Num growth patterns")).toInt();
        pop->Grow()->setNum_patterns(num);
    }
        // morphs or platoons
    if (c_file->getOkay() && !c_file->getStop()) {
        num = c_file->get_next_value(QString("Num platoons")).toInt();
        pop->Grow()->setNum_morphs(num); // 1, 3, and 5 are best, normal dist set
        num = pop->Grow()->getNum_morphs();
        pop->Grow()->setMorph_within_ratio(1.0);
        pop->Grow()->setMorph_dist(0, 1.0);
    }
    if (c_file->getOkay() && !c_file->getStop()) {
        if (num > 1)
        {
            temp_float = c_file->get_next_value(QString("Morph within/between ratio")).toFloat();
            pop->Grow()->setMorph_within_ratio (temp_float);
            temp_float = c_file->get_next_value(QString("Morph dist")).toFloat();
            if ((int)temp_float != -1)
            {
                float total = temp_float;
                pop->Grow()->setMorph_dist(0, temp_float);
                for (i = 1; i < num; i++)
                {
                    temp_float = c_file->get_next_value(QString("Morph dist")).toFloat();
                    total += temp_float;
                    pop->Grow()->setMorph_dist(i, temp_float);
                }
                if (total != 1.0)
                {
                    for (i = 0; i < num; i++) // normalizing values so total = 1.0
                    {
                        temp_float = pop->Grow()->getMorph_dist(i) / total;
                        pop->Grow()->setMorph_dist(i, temp_float);
                    }
                }
            }
            else  // normal dist is the default, ignore other values
            {
                for (i = 1; i < num; i++)
                {
                    temp_float = c_file->get_next_value(QString("Morph dist - ignored")).toFloat();
                }
                pop->Grow()->setMorphDist();
            }
        }
    }
#ifdef DEBUG_CONTROL
    QMessageBox::information(nullptr, "Information - reading contol file", "Morphs/platoons read.");
#endif

        // recruitment distribution designs
    if (c_file->getOkay() && !c_file->getStop()) {
        index = c_file->getIntValue(QString("Recruitment distribution method"), 2, 4, 1);
        pop->SR()->setDistribMethod(index);
        temp_int = c_file->getIntValue(QString("Recruitment distribution area (future)"), 1, 2, 1);
        pop->SR()->setDistribArea(temp_int);
        num = c_file->get_next_value(QString("Num settle assigns")).toInt(); // num settle assignments
        pop->SR()->setNumAssignments(num);
        temp_int = c_file->get_next_value(QString("Future feature")).toInt(); //
        pop->SR()->setDoRecruitInteract(temp_int);
/*        if (index == 2) {
            QString msg("Recruitment distributioin method 3 is simpler than method 2\nand takes 1 parameter for each settlement.");
            QMessageBox::information(nullptr, QString("Information recruitment distribution"), msg);
        }
        else if (index != 4 && num == 1) {
            QString msg("This model has only one settlement event. Changing to\nrecr_dist_method 4 and removing the recruitment params\n");
            msg.append("at the end of the MG params will produce identical\nresults and simplify the model.");
            QMessageBox::information(nullptr, QString("Information recruitment distribution"), msg);
        }*/
    }
    // settlement pattern
    if (c_file->getOkay() && !c_file->getStop()) {
        if (num == 0) {
            num_settle_timings = 1;
        }
        for (i = 0; i < num; i++) // gr pat, month, area, age for each assignment
        {
            datalist.clear();
            num_settle_timings++;
            for (int j = 0; j < 4; j++)
                datalist.append(c_file->get_next_value(QString("Recr assign data")));
            pop->SR()->setAssignment(i, datalist);
        }
        datalist.clear();
    }
#ifdef DEBUG_CONTROL
    QMessageBox::information(nullptr, "Information - reading contol file", "Recruitment read.");
#endif

        // movement definitions
    if (c_file->getOkay() && !c_file->getStop()) {
        pop->Move()->setNumDefs(0);
        pop->Move()->setFirstAge(0);
        if (data->get_num_areas() > 1)
        {
            num = c_file->get_next_value(QString("Num move defs")).toInt();
            pop->Move()->setNumDefs(num);
            if (num > 0)
            {
                temp_float = c_file->get_next_value(QString("Move first age")).toFloat();
                pop->Move()->setFirstAge(temp_float);
                for (i = 0; i < num; i++)
                {
                    datalist.clear();
                    for (int j = 0; j < 6; j++)
                        datalist.append(c_file->get_next_value(QString("Move definition")));
                    pop->Move()->setDefinition(i, datalist);
                }
            }
        }
    }
#ifdef DEBUG_CONTROL
    QMessageBox::information(nullptr, "Information - reading contol file", "Movement read.");
#endif

        // time block definitions
    if (c_file->getOkay() && !c_file->getStop()) {
        num = c_file->get_next_value(QString("Num block patterns")).toInt();
        data->setNumBlockPatterns(num);
        if (num > 0)
        {
            for (i = 0; i < num; i++)
            {
                temp_int = c_file->get_next_value(QString("Num blocks")).toInt();
                data->getBlockPattern(i)->setNumBlocks(temp_int);
            }
            for (i = 0; i < num; i++)
            {
                for (int j = 0; j < data->getBlockPattern(i)->getNumBlocks(); j++)
                {
                    datalist.clear();
                    datalist.append(c_file->get_next_value(QString("block start")));
                    datalist.append(c_file->get_next_value(QString("block end")));
                    data->getBlockPattern(i)->setBlock(j, datalist);
                }
            }
        }
    }
#ifdef DEBUG_CONTROL
    QMessageBox::information(nullptr, "Information - reading contol file", "Time blocks read.");
#endif
        // controls for time-varying params
    if (c_file->getOkay() && !c_file->getStop()) {
        temp_int = c_file->getIntValue(QString("Adjustment method for all time varying params"), 1, 3, 1);
        pop->Grow()->setTimeVaryMethod(temp_int);
        temp_int = c_file->getIntValue(QString("Autogenerate Biology time-varying params"), 0, 2, 0);
        pop->Grow()->setTimeVaryReadParams(temp_int);
        temp_int = c_file->getIntValue(QString("Autogenerate Spawn-Recr time-varying params"), 0, 2, 0);
        pop->SR()->setTimeVaryReadParams(temp_int);
        temp_int = c_file->getIntValue(QString("Autogenerate Fleet Q time-varying params"), 0, 2, 0);
        for (i = 0; i < data->get_num_fleets(); i++)
            data->getFleet(i)->setQTimeVaryReadParams(temp_int);
        temp_int = c_file->getIntValue(QString("Autogenerate Tagging time-varying params"), 0, 2, 0);
        data->setTagTimeVaryReadParams(temp_int); // for future capability
        temp_int = c_file->getIntValue(QString("Autogenerate Fleet selex time-varying params"), 0, 2, 0);
        for (i = 0; i < data->get_num_fleets(); i++)
            data->getFleet(i)->setSelTimeVaryReadParams(temp_int);
    }

    if (c_file->getOkay() && !c_file->getStop()) {
        // natural Mort
        temp_int = c_file->getIntValue(QString("Natural Mortality type"), 0, 4, 0);
        pop->Grow()->setNatural_mortality_type(temp_int);
        datalist.append(temp_string);
        switch (temp_int)
        {
        default:
        case 0:
            num_vals = 1;  // 1 parameter for each gender only
            break;
        case 1:
            num_vals = c_file->get_next_value(QString("Num breakpts")).toInt(); // num breakpoints
            pop->Grow()->setNatMortNumBreakPts(num_vals);
            datalist.clear();
            for (int i = 0; i < num_vals; i++) // vector of breakpoints
                datalist.append(c_file->get_next_value(QString("Breakpts")));
            pop->Grow()->setNatMortBreakPts(datalist);
//            num_vals = num_vals; // read N params for each gender for each GP
            break;
        case 2:
            temp_int = c_file->get_next_value(QString("Lorenz ref age")).toInt(); // ref age for Lorenzen
            pop->Grow()->setNaturnalMortLorenzenRefMin(temp_int);
            num_vals = 1; // read 1 param for each gender for each GP
            break;
        case 3:
        case 4:
            // age-specific M values by sex by growth pattern
            num = pop->Grow()->getNum_patterns();
            num_vals = data->get_num_ages() + 1;
            for (int i = 0; i < num; i++) // first, female M for each growth pattern
            {
                pop->Grow()->getPattern(i)->getNatMAges()->setColumnCount(num_vals);
                datalist.clear();
                for (int j = 0; j < num_vals; j++)
                {
                    datalist.append(c_file->get_next_value(QString("age-spec Mort vals female")));
                }
                pop->Grow()->getPattern(i)->setNatMFemAgeList(datalist);
                pop->Grow()->getPattern(i)->setNatMFemAgeHeader(QString("Fem_M_GP%1").arg(QString::number(i+1)));
            }
            if (data->get_num_genders() > 1)
            {
                for (i = 0; i < num; i++) // now male M for each growth pattern
                {
                    datalist.clear();
                    for (int j = 0; j < num_vals; j++)
                    {
                        datalist.append(c_file->get_next_value(QString("age-spec Mort vals male")));
                    }
                    pop->Grow()->getPattern(i)->setNatMMaleAgeList(datalist);
                    pop->Grow()->getPattern(i)->setNatMMaleAgeHeader(QString("Mal_M_GP%1").arg(QString::number(i+1)));
                }
            }
            else {
                for (int i = 0; i < num; i++) // no Male age values
                {
                    pop->Grow()->getPattern(i)->getNatMAges()->setRowCount(1);
                }
            }
            num_vals = 0; // read no additional parameters
            break;
        case 5:
            msg = QString("5 is experimental only");
            QMessageBox::information(nullptr, QString("Natural mortality option"), msg);
            temp_int = c_file->get_next_value(QString("Maunder natM option")).toInt(); // ref age for Lorenzen
            pop->Grow()->setNaturnalMortLorenzenRefMin(temp_int);
            if (temp_int == 3)
                num_vals = 6;
            else
                num_vals = 4;
            break;
        case 6:
            temp_int = c_file->get_next_value(QString("Lorenz ref min age")).toInt(); // ref min age for Lorenzen
            pop->Grow()->setNaturnalMortLorenzenRefMin(temp_int);
            temp_int = c_file->get_next_value(QString("Lorenz ref max age")).toInt(); // ref max age for Lorenzen
            pop->Grow()->setNaturnalMortLorenzenRefMax(temp_int);
            num_vals = 1; // read 1 param for each gender for each GP
            break;
        }
        for (i = 0; i < pop->Grow()->getNum_patterns(); i++)
        {
            pop->Grow()->getPattern(i)->setNumNatMParams(num_vals);
        }
    }

        // growth model
    if (c_file->getOkay() && !c_file->getStop()) {
        temp_int = c_file->getIntValue(QString("Growth Model"), 1, 8, 1);
        pop->Grow()->setModel(temp_int);
        if (temp_int == 6 || temp_int == 7) {
            QString msg(QString("Option %1 is not implemented. File reading will terminate.").arg(temp_int));
            QMessageBox::information(nullptr, "Growth model", msg);
            c_file->setOkay(false);
        }
        temp_float = c_file->get_next_value(QString("age for L1")).toFloat();
        pop->Grow()->setAge_for_l1(temp_float);
        temp_float = c_file->get_next_value(QString("age for L2")).toFloat();
        pop->Grow()->setAge_for_l2(temp_float);
    }

    if (c_file->getOkay() && !c_file->getStop()) {
        // Exponential decay for growth above max age
        temp_float = c_file->get_next_value(QString("Exponential decay")).toFloat();
        pop->Grow()->setExpDecay(temp_float);

        // future feature
        temp_float = c_file->get_next_value(QString("Future feature")).toFloat();
        pop->Grow()->setFeature(temp_float);

        // K multipliers
        if (pop->Grow()->getModel() == 3)
        {
            temp_int = c_file->get_next_value(QString("Num of K multipliers.")).toInt();
            pop->Grow()->setNumKmults(temp_int);
            temp_float = c_file->get_next_value(QString("age min for K")).toFloat();
            pop->Grow()->setAgeMin_for_K(temp_float);
            temp_float = c_file->get_next_value(QString("age max for K")).toFloat();
            pop->Grow()->setAgeMax_for_K(temp_float);
        }
        else if (pop->Grow()->getModel() == 4 || pop->Grow()->getModel() == 5)
        {
            temp_int = c_file->get_next_value(QString("Num of K multipliers.")).toInt();
            pop->Grow()->setNumKmults(temp_int);
            temp_float = c_file->get_next_value(QString("age max for K")).toFloat();
            pop->Grow()->setAgeMax_for_K(temp_float);
            temp_float = c_file->get_next_value(QString("age second for K")).toFloat();
            pop->Grow()->setAgeMid_for_K(temp_float);
            temp_float = c_file->get_next_value(QString("age min for K")).toFloat();
            pop->Grow()->setAgeMin_for_K(temp_float);
        }
    }

    if (c_file->getOkay() && !c_file->getStop()) {
        // SD add to LAA
        temp_float = c_file->get_next_value(QString("Std dev add to len at age")).toFloat();
        pop->Grow()->setSd_add(temp_float);
        // CV growth pattern
        temp_int = c_file->getIntValue(QString("CV Pattern"), 0, 4, 1);
        pop->Grow()->setCv_growth_pattern(temp_int);
    }

    if (c_file->getOkay() && !c_file->getStop()) {
        // maturity
        temp_int = c_file->getIntValue(QString("Maturity option"), 1, 6, 2);
        pop->Grow()->setMaturity_option(temp_int);
        if (temp_int == 3 ||
            temp_int == 4)
        {
            datalist.clear();
            num = data->get_num_ages() + 1; // num_ages() + 1;
            for (i = 0; i < num; i++)
                datalist.append(c_file->get_next_value(QString("age specific maturity")));
            pop->Grow()->setNumMatAgeValues(datalist.count());
            pop->Grow()->setMatAgeVals(datalist);
        }
        else if (temp_int == 6)
        {
            datalist.clear();
            num = data->getPopulation()->Grow()->getNumGrowthBins();
            for (i = 0; i < num; i++)
                datalist.append(c_file->get_next_value(QString("age specific maturity")));
            pop->Grow()->setNumMatAgeValues(datalist.count());
            pop->Grow()->setMatAgeVals(datalist);
        }

        temp_float = c_file->get_next_value(QString("First mature age")).toFloat();
        pop->Grow()->setFirst_mature_age(temp_float);
    }

        // fecundity
    if (c_file->getOkay() && !c_file->getStop()) {
        temp_int = c_file->getIntValue(QString("Fecundity option"), 1, 5, 1);
        pop->Fec()->setMethod(temp_int);

        temp_int = c_file->getIntValue(QString("Hermaphroditism option"), -1, 1, 0);
        pop->Fec()->setHermaphroditism(temp_int);
        if (temp_int != 0)
        {
            temp_int = c_file->get_next_value(QString("Hermaphroditism season")).toInt();
            pop->Fec()->setHermSeason(temp_int);
            temp_int = c_file->getIntValue(QString("Include males in Spawn Bmass"), 0, 1, 1);
            pop->Fec()->setHermIncludeMales(temp_int);
        }

        temp_int = c_file->getIntValue(QString("Parameter offset method"), 1, 3, 2);
        pop->Grow()->setParam_offset_method(temp_int);
    }

    if (c_file->getOkay() && !c_file->getStop()) {
        // mortality growth parameters
        num = 0;
        index = 0;
        // female parameters
        for (i = 0; i < pop->Grow()->getNum_patterns(); i++)
        {
            growthPattern *gp = pop->Grow()->getPattern(i);
            QString gpstr(QString("GP_%1").arg(QString::number(i+1)));
            // nat Mort
            temp_int = pop->Grow()->getNatural_mortality_type();
            switch (temp_int)
            {
            case 0:
            case 2:
            case 8:
                gp->setNumNatMParams(1);
                datalist = readParameter (c_file);
                gp->setFemNatMParam(0, datalist);
                gp->setFemNatMParamHeader(0, QString("NatM_p_1_Fem_%1").arg(gpstr));
                break;
            case 1:
                num_vals = pop->Grow()->getNatMortNumBreakPts();
                gp->setNumNatMParams(num_vals);
                for (int j = 0; j < num_vals; j++)
                {
                    datalist = readParameter (c_file);
                    gp->setFemNatMParam(j, datalist);
                    gp->getFemNatMParams()->setRowHeader(j, QString("NatM_break_%1_Fem_%2").arg(QString::number(j+1), gpstr));
                    index++;
                }
                break;
            case 3:
            case 4:
                break;
            }
            gp->setNumGrowthParams(3);

            datalist = readParameter(c_file); // L at Amin
            gp->setFemGrowthParam(0, datalist);
            gp->setFemGrowthParamHeader(0, QString("L_at_Amin_Fem_%1").arg(gpstr));
            datalist = readParameter(c_file); // L at Amax
            gp->setFemGrowthParam(1, datalist);
            gp->setFemGrowthParamHeader(1, QString("L_at_Amax_Fem_%1").arg(gpstr));
            datalist = readParameter(c_file); // von Bertalanffy
            gp->setFemGrowthParam(2, datalist);
            gp->setFemGrowthParamHeader(2, QString("VonBert_K_Fem_%1").arg(gpstr));
            if (pop->Grow()->getModel() == 2)
            {
                datalist = readParameter(c_file); // Richards coefficient
                gp->setFemGrowthParam(3, datalist);
                gp->setFemGrowthParamHeader(3, QString("Richards_Fem_%1").arg(gpstr));
            }
            else if (pop->Grow()->getModel() == 3)
            {
                int k = 3;
                for (; k < data->get_num_ages(); k++)
                {
                    datalist = readParameter(c_file); // K deviations per age
                    gp->setFemGrowthParam(k, datalist);
                    gp->setFemGrowthParamHeader(k, QString("Dev_age_%1_Fem_%2").arg(QString::number(k+1),gpstr));
                }
                gp->getFemGrowthParams()->setRowCount(k);
            }
            else if (pop->Grow()->getModel() == 8)
            {
                datalist = readParameter(c_file); // Cessation
                gp->setFemGrowthParam(3, datalist);
                gp->setFemGrowthParamHeader(3, QString("Cessation_Fem_%1").arg(gpstr));
            }
            gp->setNumCVParams(2);
            datalist = readParameter(c_file); // CV young
            gp->setFemCVParam(0, datalist);
            gp->getFemCVParams()->setRowHeader(0, QString("SD_young_Fem_%1").arg(gpstr));
            datalist = readParameter(c_file); // CV old
            gp->setFemCVParam(1, datalist);
            gp->getFemCVParams()->setRowHeader(1, QString("SD_old_Fem_%1").arg(gpstr));

            gp->setNumWtLenParams(2);
            datalist = readParameter(c_file); // fem_wt_len_1
            gp->setFemWtLenParam(0, datalist);
            gp->setFemWtLenParamHeader(0, QString("Wtlen_1_Fem_%1").arg(gpstr));

            datalist = readParameter(c_file); // fem_wt_len_2
            gp->setFemWtLenParam(1, datalist);
            gp->setFemWtLenParamHeader(1, QString("Wtlen_2_Fem_%1").arg(gpstr));

            datalist = readParameter(c_file); // fem_mat_inflect
            gp->setFemMatureParam(0, datalist);
            gp->getFemMatureParams()->setRowHeader(0, QString("Mat50%_Fem_%1").arg(gpstr));

            datalist = readParameter(c_file); // fem_mat_slope
            gp->setFemMatureParam(1, datalist);
            gp->getFemMatureParams()->setRowHeader(1, QString("Mat_slope_Fem_%1").arg(gpstr));

            datalist = readParameter(c_file); // fem_fec_alpha
            gp->setFemMatureParam(2, datalist);
            gp->getFemMatureParams()->setRowHeader(2, QString("Eggs/kg_inter_Fem_%1").arg(gpstr));

            datalist = readParameter(c_file); // fem_fec_beta
            gp->setFemMatureParam(3, datalist);
            gp->getFemMatureParams()->setRowHeader(3, QString("Eggs/kg_slope_wt_Fem_%1").arg(gpstr));
        }
    }

    if (c_file->getOkay() && !c_file->getStop()) {

        if (data->get_num_genders() > 1)
        {
            // male parameters
            num = 0;
            for (i = 0; i < pop->Grow()->getNum_patterns(); i++)
            {
                growthPattern *gp = pop->Grow()->getPattern(i);
                QString gpstr(QString("GP_%1").arg(QString::number(i+1)));
                temp_int = pop->Grow()->getNatural_mortality_type();
                switch (temp_int)
                {
                case 0:
                case 2:
                    datalist = readParameter (c_file);
                    gp->setMaleNatMParam(0, datalist);
                    gp->setMaleNatMParamHeader(0, QString("NatM_p_1_Mal_%1").arg(gpstr));
                    break;
                case 1:
                    num_vals = pop->Grow()->getNatMortNumBreakPts();
                    for (int j = 0; j < num_vals; j++)
                    {
                        datalist = readParameter (c_file);
                        gp->setMaleNatMParam(j, datalist);
                        gp->setMaleNatMParamHeader(j, QString("NatM_break_%1_Mal_%2").arg(QString::number(j+1), gpstr));
                    }
                    break;
                case 3:
                case 4:
                    break;
                }

                datalist = readParameter(c_file); // L at Amin
                gp->setMaleGrowthParam(0, datalist);
                gp->setMaleGrowthParamHeader(0, QString("L_at_Amin_Mal_%1").arg(gpstr));

                datalist = readParameter(c_file); // L at Amax
                gp->setMaleGrowthParam(1, datalist);
                gp->setMaleGrowthParamHeader(1, QString("L_at_Amax_Mal_%1").arg(gpstr));

                datalist = readParameter(c_file); // von Bertalanffy
                gp->setMaleGrowthParam(2, datalist);
                gp->setMaleGrowthParamHeader(2, QString("VonBert_K_Mal_%1").arg(gpstr));

                if (pop->Grow()->getModel() == 2)
                {
                    datalist = readParameter(c_file); // Richards coefficient
                    gp->addMaleGrowthParam(datalist);
                    gp->setMaleGrowthParamHeader(3, QString("Richards_Mal_%1").arg(gpstr));
                    num++;
                }
                if (pop->Grow()->getModel() == 3)
                {
                    int k = 3;
                    for (; k < data->get_num_ages(); k++)
                    {
                        datalist = readParameter(c_file); // K deviations per age
                        gp->setMaleGrowthParam(k, datalist);
                        gp->setMaleGrowthParamHeader(k, QString("Dev_age_%1_Mal_%2").arg(QString::number(k+1),gpstr));
                    }
                    gp->getMaleGrowthParams()->setRowCount(k);
                }
                else if (pop->Grow()->getModel() == 8)
                {
                    datalist = readParameter(c_file); // Cessation
                    gp->setMaleGrowthParam(3, datalist);
                    gp->setMaleGrowthParamHeader(3, QString("Cessation_Mal_%1").arg(gpstr));
                }
                datalist = readParameter(c_file); // SD young
                gp->setMaleCVParam(0, datalist);
                gp->setMaleCVParamHeader(0, QString("SD_young_Mal_%1").arg(gpstr));
                datalist = readParameter(c_file); // SD old
                gp->setMaleCVParam(1, datalist);
                gp->setMaleCVParamHeader(1, QString("SD_old_Mal_%1").arg(gpstr));

                datalist = readParameter(c_file); // male_wt_len_1
                gp->setMaleWtLenParam(0, datalist);
                gp->setMaleWtLenParamHeader(0, QString("Wtlen_1_Mal_%1").arg(gpstr));
                datalist = readParameter(c_file); // male_wt_len_2
                gp->setMaleWtLenParam(1, datalist);
                gp->setMaleWtLenParamHeader(1, QString("Wtlen_2_Mal_%1").arg(gpstr));
            }
        }
        if (QString(datalist.last()).compare(QString("EOF")) == 0)
            return false;
    }

    if (c_file->getOkay() && !c_file->getStop()) {
        if (pop->Fec()->getHermaphroditism())
        {
            for (i = 0; i < pop->Grow()->getNum_patterns(); i++)
            {
                growthPattern *gp = pop->Grow()->getPattern(i);
            datalist = readParameter(c_file); // hermaph_inflect
            gp->setHermaphParam(0, datalist);
            gp->getHermaphParams()->setRowHeader(0, QString("Hermaph_inflect_age"));
            datalist = readParameter(c_file); // hermaph_sd
            gp->setHermaphParam(1, datalist);
            gp->getHermaphParams()->setRowHeader(1, QString("Hermaph_std_dev"));
            datalist = readParameter(c_file); // hermaph_asymptotic
            gp->setHermaphParam(2, datalist);
            gp->getHermaphParams()->setRowHeader(2, QString("Hermaph_asymp_rate"));
            }
        }
    }

    if (c_file->getOkay() && !c_file->getStop()) {
        index = pop->SR()->getDistribMethod();
        if (index == 2)
        {
            num_vals = pop->Grow()->getNum_patterns() + data->get_num_areas() + pop->SR()->getNumAssignTimings();//data->get_num_seasons();
            pop->SR()->setNumDistParams(num_vals);
            for (i = 0; i < pop->Grow()->getNum_patterns(); i++)
            {
                datalist = readParameter(c_file); // recr apportion main
                pop->SR()->setDistParam(i, datalist);
                pop->SR()->getDistParams()->setRowHeader(i, QString("RecrDist_GP_%1").arg(QString::number(i+1)));
            }
            for (num = 0; num < data->get_num_areas(); num++, i++)
            {
                datalist = readParameter(c_file); // recr apportion to areas
                pop->SR()->setDistParam(i, datalist);
                pop->SR()->getDistParams()->setRowHeader(i, QString("RecrDist_Area_%1").arg(QString::number(num+1)));
            }
            for (num = 0; num < pop->SR()->getNumAssignTimings(); num++, i++)
            {
                datalist = readParameter(c_file); // recr apportion to settlement events
                pop->SR()->setDistParam(i, datalist);
                pop->SR()->getDistParams()->setRowHeader(i, QString("RecrDist_Timing_%1").arg(QString::number(num+1)));
            }
            pop->SR()->setNumDistParams(i);
        }
        else if (index == 3)
        {
            num_vals = pop->SR()->getNumAssignments();
            for (i = 0; i < num_vals; i++)
            {
                datalist = readParameter(c_file); // recr apportion settlement evenets
                pop->SR()->setDistParam(i, datalist);
                pop->SR()->getDistParams()->setRowHeader(i, QString("RecrDist_Assignment_%1").arg(QString::number(i+1)));
            }
            pop->SR()->setNumDistParams(i);
        }
    }

#ifdef DEBUG_CONTROL
    QMessageBox::information(nullptr, "Information - reading contol file", "Growth params read.");
#endif

    if (c_file->getOkay() && !c_file->getStop()) {
        // Cohort growth dev base
        datalist = readParameter(c_file);  //
        pop->Grow()->setCohortParam(datalist);
        pop->Grow()->getCohortParams()->setRowHeader(0, QString("CohortGrowDev"));
    }

    if (c_file->getOkay() && !c_file->getStop()) {
        // movement parameters (2 per definition)
        num = pop->Move()->getNumDefs();
        for (i = 0; i < num; i++)
        {
            QStringList def = pop->Move()->getDefinition(i);
            temp_string = QString("seas%1_GP%2_from_%3to%4").arg(
                        def.at(0), def.at(1), def.at(2), def.at(3));
            int par = i * 2;
            datalist = readParameter(c_file);    // parameter A
            pop->Move()->setParameter (par, datalist);
            pop->Move()->setParamHeader(par, QString("MoveParm_A_%1").arg(temp_string));
            datalist = readParameter(c_file);    // parameter B
            pop->Move()->setParameter (par + 1, datalist);
            pop->Move()->setParamHeader(par+1, QString("MoveParm_B_%1").arg(temp_string));
        }
    }

    if (c_file->getOkay() && !c_file->getStop()) {
        // Platoon StDev Ratio
        if (pop->Grow()->getNum_morphs() > 1 && pop->Grow()->getMorph_within_ratio() < 0)
        {
            datalist = readParameter(c_file);    // Morph within StDev
            pop->Grow()->setMorphDistStDev(datalist);
        }
    }

    if (c_file->getOkay() && !c_file->getStop()) {
        // ageing error if requested
        if (data->get_age_composition()->getUseAgeKeyZero() >= 0)
        {
            for (i = 0; i < 7; i++)
            {
                datalist = readParameter(c_file); // parameters for age error matrix
                data->get_age_composition()->setErrorParam(i, datalist);
            }
        }
    }
        // Catch mult
    if (c_file->getOkay() && !c_file->getStop()) {
        for (i = 0; i < data->get_num_fleets(); i++)
        {
            Fleet *fleet = data->getFleet(i);
            if (fleet->getCatchMultiplier() > 0)
            {
                datalist = readParameter(c_file);
                fleet->setCatchMultParam(datalist);
            }
        }
    }

        // Fraction Female parameters
    if (c_file->getOkay() && !c_file->getStop()) {
        for (i = 0; i < pop->Grow()->getNum_patterns(); i++)
        {
            growthPattern *gp = pop->Grow()->getPattern(i);
            datalist = readParameter(c_file);
            gp->setFractionFemaleParam(datalist);
            gp->getFractionFemaleParams()->setRowHeader(0, QString("FracFemale_GP_%1").arg(QString::number(i+1)));
        }
    }

        // timevary MG Parameters
    if (c_file->getOkay() && !c_file->getStop()) {
        timevaryread = pop->Grow()->getTimeVaryReadParams();
        if (timevaryread > 0)
        {
            growthPattern *gp;
            for (i = 0; i < pop->Grow()->getNum_patterns(); i++)
            {
                gp = pop->Grow()->getPattern(i);

                readTimeVaryParams(c_file, data, gp->getFemNatMParams(), timevaryread, gp->getFemNatMTVParams());
                readTimeVaryParams(c_file, data, gp->getFemGrowthParams(), timevaryread, gp->getFemGrowthTVParams());
                readTimeVaryParams(c_file, data, gp->getFemCVParams(), timevaryread, gp->getFemCVTVParams());
                readTimeVaryParams(c_file, data, gp->getFemWtLenParams(), timevaryread, gp->getFemWtLenTVParams());
                readTimeVaryParams(c_file, data, gp->getFemMatureParams(), timevaryread, gp->getFemMatureTVParams());
            }
            if (data->get_num_genders() > 1)
            {
                for (i = 0; i < pop->Grow()->getNum_patterns(); i++)
                {
                    gp = pop->Grow()->getPattern(i);
                    readTimeVaryParams(c_file, data, gp->getMaleNatMParams(), timevaryread, gp->getMaleNatMTVParams());
                    readTimeVaryParams(c_file, data, gp->getMaleGrowthParams(), timevaryread, gp->getMaleGrowthTVParams());
                    readTimeVaryParams(c_file, data, gp->getMaleCVParams(), timevaryread, gp->getMaleCVTVParams());
                    readTimeVaryParams(c_file, data, gp->getMaleWtLenParams(), timevaryread, gp->getMaleWtLenTVParams());
                }
            }
            for (i = 0; i < pop->Grow()->getNum_patterns(); i++)
            {
                gp = pop->Grow()->getPattern(i);
                readTimeVaryParams(c_file, data, gp->getHermaphParams(), timevaryread, gp->getHermaphTVParams());
            }
            readTimeVaryParams(c_file, data, pop->SR()->getDistParams(), timevaryread, pop->SR()->getDistTVParams());
            readTimeVaryParams(c_file, data, pop->Grow()->getCohortParams(), timevaryread, pop->Grow()->getCohortTVParams());
            readTimeVaryParams(c_file, data, pop->Move()->getMovementParams(), timevaryread, pop->Move()->getMoveTVParams());
            for (i = 0; i < pop->Grow()->getNum_patterns(); i++)
            {
                gp = pop->Grow()->getPattern(i);
                readTimeVaryParams(c_file, data, gp->getFractionFemaleParams(), timevaryread, gp->getFracFmTVParams());
            }
            if (data->get_age_composition()->getUseAgeKeyZero() >= 0)
            {
                compositionAge *agec = data->get_age_composition();
                readTimeVaryParams(c_file, data, agec->getErrorParameters(), timevaryread, agec->getErrorTVParameters());
            }
        }
    }
#ifdef DEBUG_CONTROL
    QMessageBox::information(nullptr, "Information - reading contol file", "Time vary params read.");
#endif

        // seasonal_effects_on_biology_parms
    if (c_file->getOkay() && !c_file->getStop()) {
        datalist.clear();
        num = 0;
        for (i = 0; i < 10; i++)
        {
            temp_int = c_file->getIntValue(QString("Seasonal Effects setup"), 0, 1, 0);
            datalist.append(QString::number(temp_int));
        }
        pop->setSeasParamSetup(datalist);
        // read the parameters
        for (i = 0; i < datalist.count(); i++)
        {
            QStringList parm;
            temp_string = datalist.at(i);
            temp_int = temp_string.toInt();
            if (temp_int != 0)
            {
                parm = readShortParameter (c_file);
                pop->setSeasonalParam(i, parm);
            }
        }
    }
#ifdef DEBUG_CONTROL
    QMessageBox::information(nullptr, "Information - reading contol file", "Seasonal effects read.");
#endif

        // Spawner-recruitment
    if (c_file->getOkay() && !c_file->getStop()) {
        temp_int = c_file->getIntValue(QString("Spawner-Recruit Relationship"), 1, 10, 3);
        pop->SR()->setMethod (temp_int);
        temp_int = c_file->getIntValue(QString("Use steepness in equil calc"), 0, 1, 1);
        pop->SR()->setUseSteepness (temp_int);
        // future feature
        temp_int = c_file->getIntValue(QString("Future feature"), 0, 1, 0);
        pop->SR()->setFeature (temp_int);
        i = 0;
        // SR params
        num = pop->SR()->getNumFullParameters();
        for (i = 0; i < num; i++)
        {
            datalist.clear();
            datalist = readParameter(c_file);
            pop->SR()->setParameter(i, datalist);
        }
        temp_int = pop->SR()->getMethod();
        if (temp_int > 1)
        {
            pop->SR()->setParameterHeader(0, QString("SR_LN(R0)"));
            pop->SR()->setParameterHeader(1, QString("SR_BH_steep"));
            if (temp_int == 5) {
                pop->SR()->setParameterHeader(2, QString("SR_R_min"));
            }
            else if (temp_int == 7) {
                pop->SR()->setParameterHeader(1, QString("SR_Zfrac"));
                pop->SR()->setParameterHeader(2, QString("SR_Beta"));
            }
            else if (temp_int == 8) {
                pop->SR()->setParameterHeader(2, QString("SR_shape_c"));
            }
            else if (temp_int == 9) {
                pop->SR()->setParameterHeader(2, QString("SR_shape_c"));
            }
            else if (temp_int == 10) {
                pop->SR()->setParameterHeader(2, QString("SR_Rick_pow"));
            }
        }
        pop->SR()->setParameterHeader(num-3, QString("SR_sigmaR"));
        pop->SR()->setParameterHeader(num-2, QString("SR_regime"));
        pop->SR()->setParameterHeader(num-1, QString("SR_autocorr"));
    }

        // SR time vary params
    if (c_file->getOkay() && !c_file->getStop()) {
        timevaryread = pop->SR()->getTimeVaryReadParams();
        readTimeVaryParams(c_file, data, pop->SR()->getFullParameters(), timevaryread, pop->SR()->getTVParameterModel()->getVarParamTable());
    }
        // Recruitment deviations
    if (c_file->getOkay() && !c_file->getStop()) {
        temp_int = c_file->getIntValue(QString("Do Recruitment deviations"), 0, 3, 1);
        pop->SR()->setRecDevCode(temp_int);
        pop->SR()->setRecDevStartYr(c_file->get_next_value(QString("Recr dev start yr")).toInt());
        pop->SR()->setRecDevEndYr(c_file->get_next_value(QString("Recr dev end yr")).toInt());
        pop->SR()->setRecDevPhase(c_file->get_next_value(QString("Recr dev phase")).toInt());
    }

        // SR advanced opts
    if (c_file->getOkay() && !c_file->getStop()) {
        temp_int = c_file->getIntValue(QString("Do Advanced Recruitment options"), 0, 1, 1);
        pop->SR()->setAdvancedOpts(temp_int != 0);
        if (pop->SR()->getAdvancedOpts())
        {
            pop->SR()->setRecDevEarlyStart(c_file->get_next_value().toInt());
            pop->SR()->setRecDevEarlyPhase(c_file->get_next_value().toInt());
            pop->SR()->setFcastRecPhase(c_file->get_next_value().toInt());
            pop->SR()->setFcastLambda(c_file->get_next_value().toFloat());
            pop->SR()->setNobiasLastEarlyYr(c_file->get_next_value().toDouble());
            pop->SR()->setFullbiasFirstYr(c_file->get_next_value().toDouble());
            pop->SR()->setFullbiasLastYr(c_file->get_next_value().toDouble());
            pop->SR()->setNobiasFirstRecentYr(c_file->get_next_value().toDouble());
            pop->SR()->setMaxBiasAdjust(c_file->get_next_value().toFloat());
            pop->SR()->setRecCycles(c_file->get_next_value().toInt());
            pop->SR()->setRecDevMin(c_file->get_next_value().toInt());
            pop->SR()->setRecDevMax(c_file->get_next_value().toInt());
            pop->SR()->setNumRecDev(c_file->get_next_value().toInt());

            pop->SR()->setNumCycleParams(pop->SR()->getRecCycles());
            for (i = 0; i < pop->SR()->getRecCycles(); i++)
            {
                datalist = readParameter(c_file);
                pop->SR()->setCycleParam(i, datalist);
            }

            for (i = 0; i < pop->SR()->getNumRecDev(); i++)
            {
                datalist.clear();
                datalist.append(c_file->get_next_value());
                datalist.append(c_file->get_next_value());
                pop->SR()->setRecruitDev(i, datalist);
            }
        }
    }
#ifdef DEBUG_CONTROL
    QMessageBox::information(nullptr, "Information - reading contol file", "Spawn-Recruit read.");
#endif

        // Fishing mortality
    if (c_file->getOkay() && !c_file->getStop()) {
        num = 0;
        int num_seas = data->get_num_seasons();
        int num_fleets = data->get_num_fleets();
        float startF = 0;
        int phaseF = 99;
        int numInputs = 1;
        pop->M()->setNumFisheries(data->get_num_fisheries());
        for (i = 0; i < num_fleets; i++)
        {
            for (int j = 1; j <= num_seas; j++)
                if (data->getFleet(i)->catch_equil(j) > 0)
                    num++;
        }
        pop->M()->setNumInitialParams(num * num_seas);
        temp_float = c_file->get_next_value(QString("F Mort ballpark")).toFloat();
        pop->M()->setBparkF(temp_float); // bparkF ;
        temp_int = c_file->get_next_value(QString("F Mort ballpark year")).toInt();
        pop->M()->setBparkYr(temp_int); // bparkYr
        temp_int = c_file->getIntValue(QString("F Mort method"), 1, 4, 3);
        pop->M()->setMethod(temp_int); // method
        temp_float = c_file->get_next_value(QString("F Mort max")).toFloat();
        pop->M()->setMaxF(temp_float); // maxF
        pop->M()->setStartF(0.0); //startF = 0;
        pop->M()->setPhase(0); // phase = 0;
        pop->M()->setNumInputs(0); // numInputs = 0;
        pop->M()->setNumTuningIters(0); // numTuningIters = 0;
        switch (pop->M()->getMethod())
        {
        case 2:
            startF = c_file->get_next_value(QString("F Mort start value")).toFloat();
            pop->M()->setStartF(startF); // startF
            phaseF = c_file->get_next_value(QString("F Mort phase")).toInt();
            pop->M()->setPhase(phaseF); // phase
            numInputs = c_file->get_next_value(QString("F Mort num inputs")).toInt();
            pop->M()->setNumInputs(numInputs); // numInputs
            break;
        case 3:
            temp_int = c_file->getIntValue(QString("F Mort num tuning iters (3-7)"), 1, 15, 3);
            pop->M()->setNumTuningIters(temp_int); // numTuningIters
            break;
        case 4:
            int fleet = 0;
            do {
                fleet = c_file->get_next_value("Fleet number").toInt();
                startF = c_file->get_next_value(QString("F Mort start value")).toFloat();
                phaseF = c_file->get_next_value(QString("F Mort phase")).toInt();
                if (fleet == -9999)
                    break;
                if (c_file->checkIntValue(fleet, "Fleet number", 1, data->get_num_fleets(), 1) < 0)
                    break;
                pop->M()->setFleetF(fleet, startF, phaseF);//, numInputs);
            } while (fleet != -9999);
            temp_int = c_file->getIntValue(QString("F Mort num tuning loops (3-7)"), 1, 15, 3);
            pop->M()->setNumTuningIters(temp_int); // numTuningIters
            break;
        }
    }

    if (c_file->getOkay() && !c_file->getStop()) {
        for (i = 0; i < pop->M()->getNumInputs(); i++)
        {
            datalist.clear();
            for (int j = 0; j < 6; j++)
            {
                datalist.append(c_file->get_next_value(QString("Mort inputs")));
            }
            pop->M()->setInputLine (i, datalist);
        }

        for (i = 0; i < pop->M()->getNumInitialParams(); i++)
        {
            datalist = readShortParameter(c_file);
            pop->M()->setInitialParam(i, datalist);
            pop->M()->getInitialParams()->setRowHeader(i, QString("Fleet%1").arg(QString::number(i+1)));
        }
    }
#ifdef DEBUG_CONTROL
    QMessageBox::information(nullptr, "Information - reading contol file", "F Mort read.");
#endif

    // Q setup
    if (c_file->getOkay() && !c_file->getStop()) {
        do
        {
            datalist.clear();
            flt = c_file->get_next_value(QString("Q setup Fleet number")).toInt();
            for (int j = 0; j < 5; j++)
                datalist.append(c_file->get_next_value(QString("Q setup")));
            if (datalist.at(0).compare(QString("EOF")) == 0)
                return false;
            if (flt == -9999)
                break;

            flt = c_file->checkIntValue(flt, QString("Q setup Fleet number"), 1, data->get_num_fleets(), 1);

            data->getFleet(flt - 1)->Q()->setSetup(datalist);
            data->getFleet(flt - 1)->setQSetupRead(true);
        } while (flt != -9999);
    }

    if (c_file->getOkay() && !c_file->getStop()) {
        for (i = 0; i < data->get_num_fleets(); i++)
        {
            if (data->getFleet(i)->getType() == Fleet::Survey ||
                data->getFleet(i)->getAbundanceCount() > 0)
            {
                if (!data->getFleet(i)->getQSetupRead() && data->getFleet(i)->abundance_count() > 0)
                    showInputMessage (QString("No Q Setup line for fleet %1: %2").arg(QString::number(i+1),data->getFleet(i)->getName()));
            }
            else
            {
                if (data->getFleet(i)->getQSetupRead())
                    showInputMessage (QString("Q Setup line for fleet %1: %2").arg(QString::number(i+1),data->getFleet(i)->getName()));
            }
        }
    }

    // Q parameters
    if (c_file->getOkay() && !c_file->getStop()) {
//        int id = 0;

        for (i = 0; i < num_fleets; i++)
        {
            Fleet *fleet = data->getFleet(i);
            datalist = fleet->Q()->getSetupTable()->getRowData(0);
            if (fleet->getQSetupRead())
            {
                // Q Link
                int link = datalist.at(0).toInt();
                if (link > 0)
                {
                    datalist = readParameter(c_file);
                    fleet->Q()->setLink(datalist);
                }
                // Q Mirror Offset
                if (fleet->Q()->getDoMirOffset())
                {
                    datalist = readParameter(c_file);
                    fleet->Q()->setMirOffset(datalist);
                }
                // Q Power
                if (fleet->Q()->getDoPower())
                {
                    datalist = readParameter(c_file);
                    fleet->Q()->setPower(datalist);
                }
                // Q Extra SD
                if (fleet->Q()->getDoExtraSD())
                {
                    datalist = readParameter(c_file);
                    fleet->Q()->setExtra(datalist);
                }
            }
        }
    }

    if (c_file->getOkay() && !c_file->getStop()) {
        // Q timevary params
        timevaryread = data->getFleet(0)->getQTimeVaryReadParams();
        for (i = 0; i < data->get_num_fleets(); i++)
        {
            q_ratio *fltq = data->getFleet(i)->Q();
            if (data->getFleet(i)->getQSetupRead())
            {
                readTimeVaryParams(c_file, data,
                           fltq->getParamTable(),
                           timevaryread, fltq->getTVParams());
            }
            temp_int = fltq->getNumTimeVaryParams();
            data->getFleet(i)->setName(data->getFleet(i)->getName());
        }
    }
#ifdef DEBUG_CONTROL
    QMessageBox::information(nullptr, "Information - reading contol file", "Fishery Q read.");
#endif

    if (c_file->getOkay() && !c_file->getStop()) {
        selectivity *sizesel = nullptr;
        selectivity *agesel = nullptr;
        // Size selectivity setup
        for (int i = 0; i < num_fleets; i++)
        {
            int minsel = 0;
            int selopt = 0;
            datalist.clear();
            temp_string = c_file->get_next_value("Size selex Pattern");
            selopt = temp_string.toInt();
            if (selopt > 100)
            {
                minsel = selopt/100;
                QMessageBox::information(nullptr, "Reading control file length selex",
                                         "Min selection not valid for length selex. Fixing ...");
                selopt = selopt - (minsel * 100);
                minsel = 0;
            }
//            if (data->getReadWtAtAge() && selopt > 0)
//                QMessageBox::information(nullptr, "Reading control file length selex",
//                                         "Min selection not valid for length selex. Fixing ...");

            datalist.append(QString::number(selopt));
            datalist.append(c_file->get_next_value("Size selex Discard"));
            datalist.append(c_file->get_next_value("Size selex Male"));
            datalist.append(c_file->get_next_value("Size selex Special"));

            sizesel = data->getFleet(i)->getSizeSelectivity();
            sizesel->disconnectSigs();
            sizesel->setNumAges(data->get_num_ages());
            sizesel->setBins(data->getPopulation()->Grow()->getGrowthBins());
            sizesel->setSetup(datalist);
            sizesel->connectSigs();
        }
        // Age selectivity setup
        for (int i = 0; i < num_fleets; i++)
        {
            int minsel = 0;
            int selopt = 0;
            datalist.clear();
            temp_string = c_file->get_next_value("Age selex Pattern");
            temp_int = temp_string.toInt();
            if (temp_int > 100) {
                minsel = temp_int/100;
                selopt = temp_int - (minsel * 100);
                datalist.append(QString::number(selopt));
            }
            else {
                datalist.append(temp_string);
            }
            if (minsel > 0)
            {
                switch (selopt)
                {
                case 12:
                case 13:
                case 14:
                case 16:
                case 18:
                case 26:
                case 27:
                    // minsel okay
                    break;
                case 17:
                case 19:
                case 44:
                case 45:
                    // minsel not used because separate control exists
                    QMessageBox::information(nullptr, "Reading control file age selex",
                                             "Min selection not used because separate control exists.");
                    minsel = 0;
                    break;
                case 20:
                    // minsel okay but be aware that a separate control for parm 5 can set sel = 1.0e-06 below a specified age
                    QMessageBox::information(nullptr, "Reading control file age selex",
                                             "Min selection okay to use.");
                    break;
                default:
                    // minsel not implemented and not relevant
                    QMessageBox::information(nullptr, "Reading control file age selex",
                                             "Min selection not relevant and not implemented for this selex.");
                    minsel = 0;
                }
            }
            datalist.append(c_file->get_next_value("Age selex Discard"));
            datalist.append(c_file->get_next_value("Age selex Male"));
            datalist.append(c_file->get_next_value("Age selex Special"));

            agesel = data->getFleet(i)->getAgeSelectivity();
            agesel->setMinSel(minsel);
            agesel->disconnectSigs();
            agesel->setNumAges(data->get_num_ages());
            agesel->setNumBinVals(data->get_num_ages());
            agesel->setSetup(datalist);
            agesel->connectSigs();
        }
        if (QString(datalist.last()).compare(QString("EOF")) == 0)
            return false;

        // read size selectivity parameters
        for (int i = 0; i < num_fleets; i++)
        {
            sizesel = data->getFleet(i)->getSizeSelectivity();
            sizesel->disconnectSigs();
            int num = sizesel->getNumParameters();
            int selOption = sizesel->getPattern();
            //read num_params parameters
            for (int j = 0; j < num; j++)
            {
                datalist = readParameter (c_file);
                if (j < 2 && (selOption == 5 || selOption == 11 || selOption > 40))
                    negateParameterPhase(datalist);
                sizesel->setParameter(j, datalist);
                sizesel->setParameterLabel(j,
                           QString("SizeSel_P%1_%2(%3)").arg(
                           QString::number(j+1),
                           data->getFleet(i)->getName(),
                           QString::number(i+1)));
            }
            // read retention and discard parameters
            num = sizesel->getNumRetainParameters();
            for (int j = 0; j < num; j++)
            {
                datalist = readParameter (c_file);
                sizesel->setRetainParameter(j, datalist);
                sizesel->setRetainParameterLabel(j,
                           QString("Retain_P%1_%2(%3)").arg(
                           QString::number(j+1),
                           data->getFleet(i)->getName(),
                           QString::number(i+1)));
            }
            num = sizesel->getNumDiscardParameters();
            for (int j = 0; j < num; j++)
            {
                datalist = readParameter (c_file);
                sizesel->setDiscardParameter(j, datalist);
                sizesel->setDiscardParameterLabel(j,
                           QString("DiscMort_P%1_%2(%3)").arg(
                           QString::number(j+1),
                           data->getFleet(i)->getName(),
                           QString::number(i+1)));
            }
            // read offset parameters
            num = sizesel->getNumMaleParameters();
            for (int j = 0; j < num; j++)
            {
                QString gen("Male");
                int offset = sizesel->getMale();
                if (offset == 2 || offset == 4)
                    gen = QString("Fem");
                datalist = readParameter (c_file);
                sizesel->setMaleParameter(j, datalist);
                sizesel->setMaleParameterLabel(j,
                           QString("%1_P%2_%3(%4)").arg(
                           gen,
                           QString::number(j+1),
                           data->getFleet(i)->getName(),
                           QString::number(i+1)));
            }
            // check special
            num = sizesel->getSpecial();
            if (sizesel->getPattern() == 7 && num != 1)
                sizesel->setSpecial(1);
            sizesel->connectSigs();
        }
        // if autogenerate is active, generate parameters
        sizesel->autogenParameters();

        // read age selectivity parameters
        for (int i = 0; i < num_fleets; i++)
        {
            agesel = data->getFleet(i)->getAgeSelectivity();
            agesel->disconnectSigs();
            int selOption = sizesel->getPattern();
            int num = agesel->getNumParameters();
            //read num_params parameters
            for (int j = 0; j < num; j++)
            {
                datalist = readParameter(c_file);
                if (j < 2 && (selOption == 11 || selOption > 40))
                    negateParameterPhase(datalist);
                agesel->setParameter(j, datalist);
                agesel->setParameterLabel(j,
                           QString("AgeSel_P%1_%2(%3)").arg(
                           QString::number(j+1),
                           data->getFleet(i)->getName(),
                           QString::number(i+1)));
            }
            // read retention and discard parameters
            num = agesel->getNumRetainParameters();
            for (int j = 0; j < num; j++)
            {
                datalist = readParameter (c_file);
                agesel->setRetainParameter(j, datalist);
                agesel->setRetainParameterLabel(j,
                           QString("Retain_P%1_%2(%3)").arg(
                           QString::number(j+1),
                           data->getFleet(i)->getName(),
                           QString::number(i+1)));
            }
            num = agesel->getNumDiscardParameters();
            for (int j = 0; j < num; j++)
            {
                datalist = readParameter (c_file);
                agesel->setDiscardParameter(j, datalist);
                agesel->setDiscardParameterLabel(j,
                           QString("DiscMort_P%1_%2(%3)").arg(
                           QString::number(j+1),
                           data->getFleet(i)->getName(),
                           QString::number(i+1)));
            }
            // read male offset
            num = agesel->getNumMaleParameters();
            for (int j = 0; j < num; j++)
            {
                datalist = readParameter (c_file);
                agesel->setMaleParameter(j, datalist);
                agesel->setMaleParameterLabel(j,
                           QString("Male_P%1_%2(%3)").arg(
                           QString::number(j+1),
                           data->getFleet(i)->getName(),
                           QString::number(i+1)));
            }
            // check special
            num = agesel->getSpecial();
            if (agesel->getPattern() == 7 && num != 1)
                agesel->setSpecial(1);
            agesel->connectSigs();
        }
        // if autogenerate is active, generate parameters
        sizesel->autogenParameters();

        if (QString(datalist.last()).compare(QString("EOF")) == 0)
            return false;

        // Dirichlet Mult parameter
        num_vals = 0;
        for (i = 0; i < data->get_num_fleets(); i++)
        {
            temp_int = data->getFleet(i)->getLengthCompErrorParm();
            if (temp_int > num_vals)
                num_vals = temp_int;
        }
        for (i = 0; i < num_vals; i++)
        {
            datalist = readParameter (c_file);
            data->get_length_composition()->setDirichletParam(i, datalist);
        }
        num_vals = 0;
        for (i = 0; i < data->get_num_fleets(); i++)
        {
            temp_int = data->getFleet(i)->getAgeCompErrorParm();
            if (temp_int > num_vals)
                num_vals = temp_int;
        }
        for (i = 0; i < num_vals; i++)
        {
            datalist = readParameter (c_file);
            data->get_age_composition()->setDirichletParam(i, datalist);
        }

        timevaryread = data->getFleet(0)->getSelTimeVaryReadParams();
        if (timevaryread > 0)
        {
            tablemodel *paramtable;
            tablemodel *varParamtable;

            // size selex time varying
            for (i = 0; i < data->get_num_fleets(); i++)
            {
                // param time varying
                sizesel = data->getFleet(i)->getSizeSelectivity();
                varParamtable = sizesel->getTimeVaryParameterModel();
                paramtable = data->getFleet(i)->getSizeSelexModel();
                readTimeVaryParams(c_file, data, paramtable, timevaryread, varParamtable);
                // retain param time varying
                paramtable = sizesel->getRetainParameterTable();
                varParamtable = sizesel->getRetainTimeVaryParameterModel();
                readTimeVaryParams(c_file, data, paramtable, timevaryread, varParamtable);
                // discard param time varying
                paramtable = sizesel->getDiscardParameterTable();
                varParamtable = sizesel->getDiscardTimeVaryParameterModel();
                readTimeVaryParams(c_file, data, paramtable, timevaryread, varParamtable);
                // male param time varying
                paramtable = sizesel->getMaleParameterTable();
                varParamtable = sizesel->getMaleTimeVaryParameterModel();
                readTimeVaryParams(c_file, data, paramtable, timevaryread, varParamtable);
            }
            // age selex time varying
            for (i = 0; i < data->get_num_fleets(); i++)
            {
                // param time varying
                agesel = data->getFleet(i)->getAgeSelectivity();
                paramtable = agesel->getParameterModel();
                varParamtable = agesel->getTimeVaryParameterModel();
                readTimeVaryParams(c_file, data, paramtable, timevaryread, varParamtable);
                // retain param time varying
                paramtable = agesel->getRetainParameterTable();
                varParamtable = agesel->getRetainTimeVaryParameterModel();
                readTimeVaryParams(c_file, data, paramtable, timevaryread, varParamtable);
                // discard param time varying
                paramtable = agesel->getDiscardParameterTable();
                varParamtable = agesel->getDiscardTimeVaryParameterModel();
                readTimeVaryParams(c_file, data, paramtable, timevaryread, varParamtable);
                // male param time varying
                paramtable = agesel->getMaleParameterTable();
                varParamtable = agesel->getMaleTimeVaryParameterModel();
                readTimeVaryParams(c_file, data, paramtable, timevaryread, varParamtable);
            }
        }
    }

#ifdef DEBUG_CONTROL
    QMessageBox::information(nullptr, "Information - reading contol file", "Selectivity read.");
#endif

        // 2D-AR1 smoother
    if (c_file->getOkay() && !c_file->getStop()) {
        temp_int = c_file->getIntValue(QString("2D_AR1 selectivity? 0/1"), 0, 1, 0);
        data->setUse2DAR1(temp_int == 1);
        if (temp_int == 1) {
            // fleet, ymin,   ymax,   amin,   amax,   sig_amax, use_rho, len1/age2, devphase, before_range, after_range
            do {
                datalist.clear();
                flt = c_file->getIntValue(QString("Fleet number"), -9999, data->get_num_fleets(), 1);
                datalist << c_file->get_next_value(QString("First year with deviations"));
                datalist << c_file->get_next_value(QString("Last year with deviations"));
                datalist << c_file->get_next_value(QString("Amin"));
                datalist << c_file->get_next_value(QString("Amax"));
                datalist << c_file->get_next_value(QString("Sigma maximum"));
                datalist << c_file->get_next_value(QString("Use Rho"));
                datalist << c_file->get_next_value(QString("Len/Age"));
                datalist << c_file->get_next_value(QString("Dev phase"));
                datalist << c_file->get_next_value(QString("Before range"));
                datalist << c_file->get_next_value(QString("After range"));
                if (datalist.first().compare("EOF") == 0)
                    break;

                if (flt != -9999) {
                    data->getFleet(flt-1)->get2DAR1()->setSpec(datalist);
                    // parameters
                    data->getFleet(flt-1)->get2DAR1()->setParam(0, readShortParameter(c_file));
                    data->getFleet(flt-1)->get2DAR1()->setParam(1, readShortParameter(c_file));
                    data->getFleet(flt-1)->get2DAR1()->setParam(2, readShortParameter(c_file));
                    emit data->getFleet(flt-1)->newDataRead();
                }
            } while (flt != -9999);
        }
    }

        // Tag loss and Tag reporting parameters go next
    if (c_file->getOkay() && !c_file->getStop()) {
        temp_int = c_file->getIntValue(QString("Read Tag parameters? 0/1"), 0, 1, 0);
        data->setTagLoss(temp_int);
        if (temp_int == 1)
        {
            int numCfleets = 0;
            for (int i = 0; i < num_fleets; i++)
                if (data->getFleet(i)->getType() == 1 ||
                        data->getFleet(i)->getType() == 2)
                    numCfleets++;
            num = data->getNumTagGroups();
            // tag loss init
            data->getTagLossInit()->setNumParams(num);
            for (i = 0; i < num; i++) {
                datalist = readParameter(c_file);
                data->getTagLossInit()->setParameter(i, datalist);
            }
            // tag loss chronic
            data->getTagLossChronic()->setNumParams(num);
            for (i = 0; i < num; i++) {
                datalist = readParameter(c_file);
                data->getTagLossChronic()->setParameter(i, datalist);
            }
            // tag overdispersion
            data->getTagOverdispersion()->setNumParams(num);
            for (i = 0; i < num; i++) {
                datalist = readParameter(c_file);
                data->getTagOverdispersion()->setParameter(i, datalist);
            }
            // tag report fleet
            data->getTagReportFleet()->setNumParams(data->get_num_fleets());
            for (i = 0; i < numCfleets; i++) {
                datalist = readParameter(c_file);
                data->getTagReportFleet()->setParameter(i, datalist);
            }
            // tag report decay
            data->getTagReportDecay()->setNumParams(data->get_num_fleets());
            for (i = 0; i < numCfleets; i++) {
                datalist = readParameter(c_file);
                data->getTagReportDecay()->setParameter(i, datalist);
            }
        }
    }
#ifdef DEBUG_CONTROL
    QMessageBox::information(nullptr, "Information - reading contol file", "Tag Params read.");
#endif

        // #_Variance_adjustments_to_input_value
    if (c_file->getOkay() && !c_file->getStop()) {
        int id = 0;
        data->setInputValueVariance(0);
        do
        {
            temp_string = c_file->get_next_value(QString("input var id"));
            id = temp_string.toInt();
            flt = c_file->get_next_value(QString("input var fleet")).toInt();
            temp_float = c_file->get_next_value(QString("input var value")).toFloat();
            if (temp_string.compare(QString("EOF")) == 0)
                return false;
            if (id == END_OF_LIST)
                break;
            data->setInputValueVariance(1);
            data->getFleet(flt-1)->setDoInputVariance(id, true);
            data->getFleet(flt-1)->setInputVarianceValue(id, temp_float);
/*            switch (id)
            {
            case 1:
                data->getFleet(flt-1)->setAddToSurveyCV(temp_float);
                break;
            case 2:
                data->getFleet(flt-1)->setAddToDiscardSD(temp_float);
                break;
            case 3:
                data->getFleet(flt-1)->setAddToBodyWtCV(temp_float);
                break;
            case 4:
                data->getFleet(flt-1)->setMultByLenCompN(temp_float);
                break;
            case 5:
                data->getFleet(flt-1)->setMultByAgeCompN(temp_float);
                break;
            case 6:
                data->getFleet(flt-1)->setMultBySAA(temp_float);
                break;
            case 7:
                data->getFleet(flt-1)->setMultByGenSize(temp_float);
                break;
            default:
                break;
            }*/
        } while (id != END_OF_LIST);
    }

        // Max lambda phase
    if (c_file->getOkay() && !c_file->getStop()) {
        temp_int = c_file->get_next_value(QString("Lambda max phase")).toInt();
        data->setLambdaMaxPhase(temp_int);
    }

        // sd offset
    if (c_file->getOkay() && !c_file->getStop()) {
        temp_int = c_file->getIntValue(QString("Lambda sd offset"), 0, 1, 0);
        data->setLambdaSdOffset(temp_int);
    }

        // lambda changes
        // component, fleet, phase, value, sizefreq method
    if (c_file->getOkay() && !c_file->getStop()) {
        data->setNumLambdaAdjustments(0);
        do
        {
            int flt = 1;
            datalist.clear();
            temp_string = c_file->get_next_value(QString("lambda component"));
            if (temp_string.contains(QString("EOF")))
                return false;
            temp_int = temp_string.toInt();
//            temp_int = c_file->checkIntValue(temp_int, QString("lambda component"), 1, 18, 4);
            switch (temp_int) {
            case 1:
            case 2:
                num_fleets = data->get_num_fleets();
                datalist.append(temp_string);
                temp_string = c_file->get_next_value(QString("Lambda fleet"));
                flt = abs(temp_string.toInt());
                flt = c_file->checkIntValue(flt, QString("Lambda fleet num"), 1, num_fleets, 1);
                datalist.append(temp_string);
                datalist.append(c_file->get_next_value(QString("lambda phase")));
                datalist.append(c_file->get_next_value(QString("lambda value")));
                datalist.append(c_file->get_next_value(QString("lambda sizefq method")));
                data->getFleet(flt-1)->appendLambda(datalist);
                break;
            case 15:
                num = data->getNumTagGroups();
                datalist.append(temp_string);
                datalist.append(c_file->get_next_value(QString("lambda fleet")));
                datalist.append(c_file->get_next_value(QString("lambda phase")));
                datalist.append(c_file->get_next_value(QString("lambda value")));
                datalist.append(c_file->get_next_value(QString("lambda sizefq method")));
                break;
            default:
                datalist.append(temp_string);
                datalist.append(c_file->get_next_value(QString("lambda fleet")));
                datalist.append(c_file->get_next_value(QString("lambda phase")));
                datalist.append(c_file->get_next_value(QString("lambda value")));
                datalist.append(c_file->get_next_value(QString("lambda sizefq method")));
            }
            if (temp_int != END_OF_LIST) {
                data->addLambdaAdjustment(datalist);
            }
        } while (temp_int != END_OF_LIST);
    }
#ifdef DEBUG_CONTROL
    QMessageBox::information(nullptr, "Information - reading contol file", "Lambdas read.");
#endif

    if (c_file->getOkay() && !c_file->getStop()) {
        int addrpt = c_file->getIntValue(QString("Additional Std dev reporting? 0-2"), 0, 2, 0);
        data->getAddSdReporting()->setActive(addrpt);
        if (addrpt > 0)
        {
            int numSel = 0, numGrw = 0, numNum = 0, numMort = 0;
            // read 4 values for Selectivity
            datalist.clear();
            for (i = 0; i < 4; i++)
                datalist.append(c_file->get_next_value());
            data->getAddSdReporting()->setSelex(datalist);
            if (!datalist.at(0).contains("0"))
                numSel = datalist.at(3).toInt();

            // read 2 values for Growth
            datalist.clear();
            for (i = 0; i < 2; i++)
                datalist.append(c_file->get_next_value());
            data->getAddSdReporting()->setGrowth(datalist);
            if (!datalist.at(0).contains("0"))
                numGrw = datalist.at(1).toInt();

            // read 3 values for Numbers-at-Age
            datalist.clear();
            for (i = 0; i < 3; i++)
                datalist.append(c_file->get_next_value());
            data->getAddSdReporting()->setNumAtAge(datalist);
            if (!datalist.at(0).contains("0"))
                numNum = datalist.at(2).toInt();

            if (addrpt > 1) {
                // read 2 values for nat Mort
                datalist.clear();
                for (i = 0; i < 2; i++)
                    datalist.append(c_file->get_next_value());
                data->getAddSdReporting()->setNatMort(datalist);
                if (!datalist.at(0).contains("0"))
                    numMort = datalist.at(1).toInt();

                temp_int = c_file->getIntValue("Dynamic B0", 0, 2, 0);
                data->getAddSdReporting()->setDynB0(temp_int);
                temp_int = c_file->getIntValue("Summary Bio", 0, 1, 0);
                data->getAddSdReporting()->setSumBio(temp_int);
            }

            // bins for selectivity
            if (!data->getAddSdReporting()->getSelex().at(0).contains("0"))
            {
                datalist.clear();
                for (i = 0; i < numSel; i++)
                    datalist.append(c_file->get_next_value());
                data->getAddSdReporting()->setSelexBins(datalist);
            }

            // ages for growth
            if (!data->getAddSdReporting()->getGrowth().at(0).contains("0"))
            {
                datalist.clear();
                if (data->getReadWtAtAge() == 0) {
                    for (i = 0; i < numGrw; i++)
                        datalist.append(c_file->get_next_value());
                    data->getAddSdReporting()->setGrowthBins(datalist);
                }
            }

            // ages for Num-at-Age
            if (!data->getAddSdReporting()->getNumAtAge().at(0).contains("0"))
            {
                datalist.clear();
                for (i = 0; i < numNum; i++)
                    datalist.append(c_file->get_next_value());
                data->getAddSdReporting()->setNumAtAgeBins(datalist);
            }

            if (addrpt > 1) {
                // ages for nat Mort
                if (!data->getAddSdReporting()->getNatMort().at(0).contains("0"))
                {
                    datalist.clear();
                    for (i = 0; i < numMort; i++)
                        datalist.append(c_file->get_next_value());
                    data->getAddSdReporting()->setNatMortBins(datalist);
                }
            }
        }
    }

    if (c_file->getOkay() && !c_file->getStop()) {
        temp_int = c_file->get_next_value().toInt();
        if (temp_int != 999)
        {
            temp_string = QString("Problem reading control file. The end-of-data token '%1' does not match '999'").arg(
                        QString::number(temp_int));
            c_file->error(temp_string);
        }
    }

        c_file->close();
    }
    else
    {
        c_file->error(QString("Control file does not exist or is unreadable."));
    }
    return 1;
}

int write33_controlFile(ss_file *c_file, ss_model *data)
{
    int temp_int, num, num_vals, chars = 0;
    int i, j;
    float temp_float = 0.0;
    double temp_doub = 0.0;
    QString line, temp_string;
    QStringList str_list;
    population * pop = data->pPopulation;

    if(c_file->open(QIODevice::WriteOnly))
    {
//        chars += c_file->writeline(QString("#V%1").arg(getDatafileVersionString()));
        chars += writeVersionComment(c_file);

        chars += c_file->write_comments();

        line = QString(QString("#_data_and_control_files: %1 // %2").arg(
                           data->getDataFileName(),
                           data->getControlFileName()));
        chars += c_file->writeline(line);

        // read wtatage.ss
        chars += c_file->write_val(data->getReadWtAtAge(), 1,
                    QString("0 means do not read wtatage.ss; 1 means read and use wtatage.ss and also read and use growth parameters"));

        // growth patterns
        chars += c_file->write_val(pop->Grow()->getNum_patterns(), 1,
                    QString ("N_Growth_Patterns (Growth Patterns, Morphs, Bio Patterns, GP are terms used interchangeably in SS)"));

        // morphs or platoons
        num = pop->Grow()->getNum_morphs();
        chars += c_file->write_val(num, 1,
                    QString("N_platoons_Within_GrowthPattern "));

        line.clear();
        if (num > 1)
        {
            temp_float = pop->Grow()->getMorph_within_ratio();
            chars += c_file->write_val(temp_float, 1, QString("Platoon_within/between_stdev_ratio (no read if N_platoons=1)"));
            line.clear();
            for (int i = 0; i < num; i++)
            {
                temp_float = pop->Grow()->getMorph_dist(i);
                line.append(QString("%1 ").arg(
                         QString::number(temp_float)));
            }
            line.append(QString("#vector_platoon_dist_(-1_in_first_val_gives_normal_approx)"));
            chars += c_file->writeline(line);
            chars += c_file->writeline("#");
        }
        else
        {
            line.append(QString("#_Cond 1 "));
            line.append(QString("#_Platoon_within/between_stdev_ratio (no read if N_platoons=1)"));
            chars += c_file->writeline(line);
            line.clear();
            line.append(QString("#_Cond 1 "));
            line.append(QString("#vector_platoon_dist_(-1_in_first_val_gives_normal_approx)"));
            chars += c_file->writeline(line);
            chars += c_file->writeline("#");
        }

        // recruitment designs
        line.clear();
        temp_int = pop->SR()->getDistribMethod();
        chars += c_file->write_val(temp_int, 1, QString("recr_dist_method for parameters:  2=main effects for GP, Area, Settle timing; 3=each Settle entity; 4=none (only when N_GP*Nsettle*pop==1)"));
        temp_int = pop->SR()->getDistribArea();
        chars += c_file->write_val(temp_int, 1, QString("not yet implemented; Future usage: Spawner-Recruitment: 1=global; 2=by area"));
        num = pop->SR()->getNumAssignments();
        chars += c_file->write_val(num, 1, QString("number of recruitment settlement assignments "));
        temp_int = pop->SR()->getDoRecruitInteract();
        chars += c_file->write_val(temp_int, 2, "unused option");

        line = QString ("#GPattern month  area  age (for each settlement assignment)");
        chars += c_file->writeline(line);
        for (i = 0; i < num; i++)
        {
            str_list = pop->SR()->getAssignment(i);
            chars += c_file->write_vector(str_list, 2);
        }
        chars += c_file->writeline("#");

        // movement definitions
        line.clear();
        if (data->get_num_areas() > 1)
        {
            num = pop->Move()->getNumDefs();
            chars += c_file->write_val(num, 1, QString("N_movement_definitions"));
            line.clear();
            if (num == 0) {
                line = QString("#_Cond 1: ");
            }
            temp_float = pop->Move()->getFirstAge();
            line.append(QString("%1 # first age that moves (real age at begin of season, not integer)").arg(QString::number(temp_float)));
            chars += c_file->writeline(line);
            line = QString("# seas GP  source dest minage maxage");
            chars += c_file->writeline(line);
            for (int i = 0; i < num; i++)
            {
                str_list = pop->Move()->getDefinition(i);
                chars += c_file->write_vector(str_list, 4);
            }
        }
        else
        {
            line = QString("#_Cond 0 # N_movement_definitions goes here if Nareas > 1");
            chars += c_file->writeline(line);
            line = QString("#_Cond 1.0 # first age that moves (real age at begin of season, not integer) also cond on do_migration>0");
            chars += c_file->writeline(line);
            line = QString("#_Cond 1 1 1 2 4 10 # example move definition for seas=1, morph=1, source=1 dest=2, age1=4, age2=10");
            chars += c_file->writeline(line);
        }
        chars += c_file->writeline("#");


        // time block patterns
        num = data->getNumBlockPatterns();
        chars += c_file->write_val(num, 1, QString("Nblock_Patterns"));
        line.clear();
        for (int i = 0; i < num; i++)
        {
            line.append(" ");
            BlockPattern *blk = data->getBlockPattern(i);
            temp_int = blk->getNumBlocks();
            line.append(QString("%1").arg(temp_int));
        }
        if (line.isEmpty())
        {
            line.append("#_Cond 0 no time blocks defined ");
            chars += c_file->writeline(line);
        }
        else
        {
            line.append(QString(" #_blocks_per_pattern "));
            chars += c_file->writeline(line);
            line = QString("# begin and end years of blocks");
            chars += c_file->writeline(line);
            line.clear();
            for (int i = 0; i < num; i++)
            {
                line.clear();
                BlockPattern *blk = data->getBlockPattern(i);
                temp_int = blk->getNumBlocks();
                for (int j = 0; j < temp_int; j++)
                    line.append(blk->getBlockText(j));
                line.append(QString(" # pattern %1").arg(QString::number(i+1)));
                chars += c_file->writeline(line);
            }
        }
        chars += c_file->writeline("#");

        // controls for time-varying parameters
        line = QString("# controls for all timevary parameters ");
        chars += c_file->writeline(line);
        temp_int = pop->Grow()->getTimeVaryMethod();
        line = QString(QString("%1 #_time-vary parm bound check (1=warn relative to base parm bounds; 3=no bound check); Also see env (3) and dev (5) options to constrain with base bounds").arg (
                           QString::number(temp_int)));
        chars += c_file->writeline(line);
        line.clear();
        chars += c_file->writeline(QString("#"));
        chars += c_file->writeline(QString("# AUTOGEN"));
        temp_int = pop->Grow()->getTimeVaryReadParams();
        line.append(QString("%1 ").arg(QString::number(temp_int)));
        temp_int = pop->SR()->getTimeVaryReadParams();
        line.append(QString("%1 ").arg(QString::number(temp_int)));
        temp_int = data->getFleet(0)->getQTimeVaryReadParams();
        line.append(QString("%1 ").arg(QString::number(temp_int)));
        temp_int = data->getTagTimeVaryReadParams();
        line.append(QString("%1 ").arg(QString::number(temp_int)));
        temp_int = data->getFleet(0)->getSelTimeVaryReadParams();
        line.append(QString("%1 ").arg(QString::number(temp_int)));
        line.append(QString("# autogen: 1st element for biology, 2nd for SR, 3rd for Q, 4th reserved, 5th for selex"));
        chars += c_file->writeline(line);
        line = QString("# where: 0 = autogen time-varying parms of this category; 1 = read each time-varying parm line; 2 = read then autogen if parm min==-12345");
        chars += c_file->writeline(line);
        chars += c_file->writeline("#");
        chars += c_file->writeline("#_Available timevary codes");
        chars += c_file->writeline("#_Block types: 0: P_block=P_base*exp(TVP); 1: P_block=P_base+TVP; 2: P_block=TVP; 3: P_block=P_block(-1) + TVP");
        chars += c_file->writeline("#_Block_trends: -1: trend bounded by base parm min-max and parms in transformed units (beware); -2: endtrend and infl_year direct values; -3: end and infl as fraction of base range");
        chars += c_file->writeline("#_EnvLinks:  1: P(y)=P_base*exp(TVP*env(y));  2: P(y)=P_base+TVP*env(y);  3: P(y)=f(TVP,env_Zscore) w/ logit to stay in min-max;  4: P(y)=2.0/(1.0+exp(-TVP1*env(y) - TVP2))");
        chars += c_file->writeline("#_DevLinks:  1: P(y)*=exp(dev(y)*dev_se;  2: P(y)+=dev(y)*dev_se;  3: random walk;  4: zero-reverting random walk with rho;  5: like 4 with logit transform to stay in base min-max");
        chars += c_file->writeline("#_DevLinks(more):  21-25 keep last dev for rest of years");
        chars += c_file->writeline("#");
        chars += c_file->writeline("#_Prior_codes:  0=none; 6=normal; 1=symmetric beta; 2=CASAL's beta; 3=lognormal; 4=lognormal with biascorr; 5=gamma");
        chars += c_file->writeline("#");

        // natM and growth
        chars += c_file->writeline("# setup for M, growth, wt-len, maturity, fecundity, (hermaphro), recr_distr, cohort_grow, (movement), (age error), (catch_mult), sex ratio ");
        chars += c_file->writeline("#_NATMORT");
        temp_int = pop->Grow()->getNatural_mortality_type();
        line = QString(QString("%1 #_natM_type:_0=1Parm; 1=N_breakpoints;_2=Lorenzen;_3=agespecific;_4=agespec_withseasinterpolate;_5=BETA:_Maunder_link_to_maturity").arg(
                           temp_int));
        chars += c_file->writeline (line);
        switch (temp_int)
        {
        case 0:
            line = QString ("  #_no additional input for selected M option; read 1P per morph");
            chars += c_file->writeline(line);
            num_vals = 1;
            break;
        case 1:
            num_vals = pop->Grow()->getNatMortNumBreakPts();
            line = QString("%1 #_N_breakpoints").arg(QString::number(num_vals));
            chars += c_file->writeline(line);
            line.clear();
            str_list = pop->Grow()->getNatMortBreakPts();
            chars += c_file->write_vector(str_list, 2, QString("age(real) at M breakpoints"));
            break;
        case 2:
            num = pop->Grow()->getNaturalMortLorenzenRefMin();
            line = QString("%1 #_Lorenzen ref age ").arg(num);
            chars += c_file->writeline(line);
            num_vals = 1;
            break;
        case 3:
        case 4:
            line = QString("#_Age_natmort_by sex x growthpattern (nest GP in sex)");
            chars += c_file->writeline(line);
            line.clear();
            str_list = pop->Grow()->getNatMortAges();
            num_vals = pop->Grow()->getNum_patterns();
            for (i = 0; i < num_vals; i++)
            {
                str_list = pop->Grow()->getPattern(i)->getNatMFemAgeList();
                chars += c_file->write_vector(str_list, 2, QString("_Fem_M_GP%1").arg(QString::number(i+1)));
            }
            if (data->get_num_genders() > 1)
                for (i = 0; i < num_vals; i++)
                {
                    str_list = pop->Grow()->getPattern(i)->getNatMMaleAgeList();
                    chars += c_file->write_vector(str_list, 2, QString("_Male_M_GP%1").arg(QString::number(i+1)));
                }
            num_vals = 0;
            break;
        case 5:
            num = pop->Grow()->getNaturalMortLorenzenRefMin();
            line = QString("%1 #_Maunder NatMort option ").arg(num);
            chars += c_file->writeline(line);
            if (num == 3)
                num_vals = 6;
            else
                num_vals = 4;
            break;
        case 6:
            num = pop->Grow()->getNaturalMortLorenzenRefMin();
            line = QString("%1 #_Lorenzen ref age min").arg(num);
            chars += c_file->writeline(line);
            num = pop->Grow()->getNaturalMortLorenzenRefMax();
            line = QString("%1 #_Lorenzen ref age max").arg(num);
            chars += c_file->writeline(line);
            num_vals = 1;
            break;
        }
        chars += c_file->writeline(QString("#"));

        // growth model
        temp_int = pop->Grow()->getModel();
        chars += c_file->write_val(temp_int, 1, QString("GrowthModel: 1=vonBert with L1&L2; 2=Richards with L1&L2; 3=age_specific_K_incr; 4=age_specific_K_decr; 5=age_specific_K_each; 6=NA; 7=NA; 8=growth cessation"));
        temp_float = pop->Grow()->getAge_for_l1();
        chars += c_file->write_val(temp_float, 1, QString("Age(post-settlement)_for_L1;linear growth below this"));
        temp_float = pop->Grow()->getAge_for_l2();
        chars += c_file->write_val(temp_float, 1, QString("Growth_Age_for_L2 (999 to use as Linf)"));
        if (temp_int == 3)
        {
            temp_float = pop->Grow()->getAgeMin_for_K();
            chars += c_file->write_val(temp_float, 1, QString("Min age for age-specific K"));
            temp_float = pop->Grow()->getAgeMax_for_K();
            chars += c_file->write_val(temp_float, 1, QString("Max age for age-specific K"));
        }
        temp_float = pop->Grow()->getExpDecay();
        chars += c_file->write_val(temp_float, 1, QString("exponential decay for growth above maxage (value should approx initial Z; -999 replicates 3.24; -998 to not allow growth above maxage)"));

        temp_float = pop->Grow()->getFeature();
        chars += c_file->write_val(temp_float, 1, QString("placeholder for future growth feature"));
        chars += c_file->writeline ("#");

        temp_float = pop->Grow()->getSd_add();
        chars += c_file->write_val(temp_float, 1, QString("SD_add_to_LAA (set to 0.1 for SS2 V1.x compatibility)"));
        temp_int = pop->Grow()->getCv_growth_pattern();
        chars += c_file->write_val(temp_int, 1, QString("CV_Growth_Pattern:  0 CV=f(LAA); 1 CV=F(A); 2 SD=F(LAA); 3 SD=F(A); 4 logSD=F(A)"));
        chars += c_file->writeline(QString("#"));

        temp_int = pop->Grow()->getMaturity_option();
        chars += c_file->write_val(temp_int, 1, QString("maturity_option:  1=length logistic; 2=age logistic; 3=read age-maturity matrix by growth_pattern; 4=read age-fecundity; 5=disabled; 6=read length-maturity"));
        if (temp_int == 3 ||  // age specific maturity
            temp_int == 4)
        {
            chars += c_file->writeline(QString("#_Age_Fecundity by growth pattern"));
            line.clear();
            str_list = pop->Grow()->getMatAgeVals();
            chars += c_file->write_vector(str_list, 1);
        }
        else if (temp_int == 6)  // length specific maturity
        {
            chars += c_file->writeline(QString("#_Length_Fecundity by growth pattern"));
            line.clear();
            str_list = pop->Grow()->getMatAgeVals();
            chars += c_file->write_vector(str_list, 1);
        }
/*        else
        {
            line = QString("#_placeholder for empirical age-maturity by growth pattern");
            chars += c_file->writeline (line);
        }*/

        temp_float = pop->Grow()->getFirst_mature_age();
        line = QString(QString("%1 #_First_Mature_Age").arg(
                           QString::number(temp_float)));
        chars += c_file->writeline (line);
        temp_int = pop->Fec()->getMethod();
        line = QString(QString("%1 #_fecundity_at_length option:(1)eggs=Wt*(a+b*Wt);(2)eggs=a*L^b;(3)eggs=a*Wt^b; (4)eggs=a+b*L; (5)eggs=a+b*W").arg(
                           temp_int));
        chars += c_file->writeline (line);
        temp_int = pop->Fec()->getHermaphroditism();
        line = QString(QString("%1 #_hermaphroditism option:  0=none; 1=female-to-male age-specific fxn; -1=male-to-female age-specific fxn").arg(
                           temp_int));
        chars += c_file->writeline (line);
        if (temp_int == 1)
        {
            temp_int = pop->Fec()->getHermSeason();
            line = QString(QString("%1 #_hermaphroditism Season:  -1 trans at end of each seas; or specific seas").arg(
                               QString::number(temp_int)));
            chars += c_file->writeline (line);
            temp_int = pop->Fec()->getHermIncludeMales();
            line = QString(QString("%1 #_include males in spawning biomass:  0=no males; 1=add males to females; xx=reserved.").arg(
                               temp_int));
            chars += c_file->writeline (line);
        }
        temp_int = pop->Grow()->getParam_offset_method();
        line = QString(QString("%1 #_parameter_offset_approach for M, G, CV_G:  1- direct, no offset**; 2- male=fem_parm*exp(male_parm); 3: male=female*exp(parm) then old=young*exp(parm)").arg(temp_int));
        chars += c_file->writeline (line);
        chars += c_file->writeline (QString("#_** in option 1, any male parameter with value = 0.0 and phase <0 is set equal to female parameter"));
        chars += c_file->writeline("#");

        // growth parameters
        line = QString("#_growth_parms");
        chars += c_file->writeline(line);
        line = QString("#_ LO HI INIT PRIOR PR_SD PR_type PHASE env_var&link dev_link dev_minyr dev_maxyr dev_PH Block Block_Fxn");
        chars += c_file->writeline(line);
        QString sex("1");
        num = pop->Grow()->getNum_patterns();
        for (i = 0; i < num; i++)
        {
            QString gpat(QString::number(i+1));
            QString gpstr (QString("GP_%1").arg(gpat));
            QString genstr ("Fem");
            QString parstr;
            growthPattern *gp = pop->Grow()->getPattern(i);
            line = QString(QString("# Sex: %1  BioPattern: %2  NatMort").arg(sex, gpat));
            chars += c_file->writeline(line);

            int numpar = 0;
                switch (pop->Grow()->getNatural_mortality_type())
                {
                case 0:
                    numpar = 1;
                    break;
                case 1:
                    numpar = pop->Grow()->getNatMortNumBreakPts();
                    break;
                case 2:
                    numpar = 2;
                    break;
                default:
                    numpar = 0;
                }

                for (int k = 0; k < numpar; k++)
                {
                    str_list = gp->getFemNatMParam(k);
                    parstr = QString (gp->getFemNatMParams()->getRowHeader(k));
                    chars += c_file->write_vector(str_list, 1, parstr);
                }
                line = QString(QString("# Sex: %1  BioPattern: %2  Growth").arg(sex, gpat));
                chars += c_file->writeline(line);
                for (int k = 0; k < gp->getNumGrowthParams(); k++)
                {
                    str_list = gp->getFemGrowthParam(k);
                    parstr = QString(gp->getFemGrowthParams()->getRowHeader(k));
                    chars += c_file->write_vector(str_list, 1, parstr);
                }
                for (int k = 0; k < gp->getFemNumCVParams(); k++)
                {
                    str_list = gp->getFemCVParam(k);
                    parstr = QString(gp->getFemCVParams()->getRowHeader(k));
                    chars += c_file->write_vector(str_list, 1, parstr);
                }
                line = QString(QString("# Sex: %1  BioPattern: %2  WtLen").arg(sex, gpat));
                chars += c_file->writeline(line);
                for (int k = 0; k < 2; k++)
                {
                    str_list = gp->getFemWtLenParam(k);
                    parstr = QString(gp->getFemWtLenParams()->getRowHeader(k));
                    chars += c_file->write_vector(str_list, 1, parstr);
                }
                line = QString(QString("# Sex: %1  BioPattern: %2  Maturity&Fecundity").arg(sex, gpat));
                chars += c_file->writeline(line);
                for (int k = 0; k < gp->getFemMatureParams()->rowCount(); k++)
                {
                    str_list = gp->getFemMatureParam(k);
                    parstr = gp->getFemMatureParams()->getRowHeader(k);
                    chars += c_file->write_vector(str_list, 1, parstr);
                }

        }

        if (data->get_num_genders() > 1)
        {
            QString sex("2");
            for (i = 0; i < num; i++)
            {
                QString gpat(QString::number(i+1));
                QString gpstr (QString("GP_%1").arg(gpat));
                QString genstr ("Mal");
                QString parstr;
                growthPattern *gp = pop->Grow()->getPattern(i);
                line = QString(QString("# Sex: %1  BioPattern: %2  NatMort").arg(sex, gpat));
                chars += c_file->writeline(line);
                int numpar = 0;
                switch (pop->Grow()->getNatural_mortality_type())
                {
                case 0:
                    numpar = 1;
                    break;
                case 1:
                    numpar = pop->Grow()->getNatMortNumBreakPts();
                    break;
                case 2:
                    numpar = 2;
                    break;
                default:
                    numpar = 0;
                }
                for (int k = 0; k < numpar; k++)
                {
                    str_list = gp->getMaleNatMParam(k);
                    parstr = QString (gp->getMaleNatMParams()->getRowHeader(k));
                    chars += c_file->write_vector(str_list, 1, parstr);
                }
                line = QString(QString("# Sex: %1  BioPattern: %2  Growth").arg(sex, gpat));
                chars += c_file->writeline(line);
                for (int k = 0; k < gp->getMaleNumGrowthParams(); k++)
                {
                    str_list = gp->getMaleGrowthParam(k);
                    parstr = QString(gp->getMaleGrowthParams()->getRowHeader(k));
                    chars += c_file->write_vector(str_list, 1, parstr);
                }
                for (int k = 0; k < gp->getMaleNumCVParams(); k++)
                {
                    str_list = gp->getMaleCVParam(k);
                    parstr = QString(gp->getMaleCVParams()->getRowHeader(k));
                    chars += c_file->write_vector(str_list, 1, parstr);
                }
                line = QString(QString("# Sex: %1  BioPattern: %2  WtLen").arg(sex, gpat));
                chars += c_file->writeline(line);
                for (int k = 0; k < 2; k++)
                {
                    str_list = gp->getMaleWtLenParam(k);
                    parstr = QString(gp->getMaleWtLenParams()->getRowHeader(k));
                    chars += c_file->write_vector(str_list, 1, parstr);
                }
            }
        }

        line = QString("# Hermaphroditism ");
        chars += c_file->writeline(line);
        if (pop->Fec()->getHermaphroditism() != 0)
        {
            for (int k = 0; k < pop->Grow()->getNum_patterns(); k++)
            {
                QString desc;
                growthPattern *gp = pop->Grow()->getPattern(k);
                for (i = 0; i < 3; i++)
                {
                    str_list = gp->getHermaphParam(i); // hermaph_inflect, sd, asymptotic
                    desc = QString(QString(" # Hermaph_p_%1_GP%2").arg(QString::number(i + 1),
                                                                       QString::number(k+1)));
                    chars += c_file->write_vector(str_list, 1, desc);
                }
            }
        }

        line = QString("#  Recruitment Distribution  ");
        chars += c_file->writeline(line);
        num_vals = pop->SR()->getNumDistParams();
        for (i = 0; i < num_vals; i++)
        {
            chars += c_file->write_vector(pop->SR()->getDistParam(i), 2,
                              pop->SR()->getDistParams()->getRowHeader(i));
        }


/*        if (pop->SR()->getDoRecruitInteract())
        {
            QString desc;
            num = pop->SR()->getNumInteractParams();
            for (i = 0; i < num; i++)
            {
                str_list = pop->SR()->getInteractParam(i);
                desc = QString(pop->SR()->getInteractParams()->getRowHeader(i));
                chars += c_file->write_vector(str_list, 1, desc);
            }
        }*/
        line = QString("#  Cohort growth dev base");
        chars += c_file->writeline(line);
        line.clear();
        str_list = pop->Grow()->getCohortParam();
        chars += c_file->write_vector(str_list, 2, "CohortGrowDev");

        // movement parameters
        line = QString("# Movement");
        chars += c_file->writeline(line);
        num = pop->Move()->getNumDefs();
        for (i = 0; i < num; i++)
        {
            str_list = pop->Move()->getDefinition(i);
            QString seas = str_list.at(0);
            QString gp = str_list.at(1);
            QString from = str_list.at(2);
            QString to = str_list.at(3);
            QString hdr;
            int par = i * 2;
            line.clear();
            str_list = pop->Move()->getParameter(par);
            hdr = pop->Move()->getMovementParams()->getRowHeader(par);
            chars += c_file->write_vector(str_list, 2, hdr);
/*            for (int l = 0; l < str_list.count(); l++)
                line.append(QString(" %1").arg(str_list.at(l))); ;
            line.append(QString(" #_%1").arg(pop->Move()->getMovementParams()->getRowHeader(par)));
            chars += c_file->writeline (line);
            line.clear();*/
            str_list = pop->Move()->getParameter(par + 1);
            hdr = pop->Move()->getMovementParams()->getRowHeader(par+1);
            chars += c_file->write_vector(str_list, 2, hdr);
/*            for (int l = 0; l < str_list.count(); l++)
                line.append(QString(" %1").arg(str_list.at(l)));
            line.append(QString(" #_%1").arg(pop->Move()->getMovementParams()->getRowHeader(par+1)));
            chars += c_file->writeline (line);*/
        }

        // Platoon StDev Ratio
        line = QString("#  Platoon StDev Ratio");
        chars += c_file->writeline(line);
        if (pop->Grow()->getNum_morphs() > 1 && pop->Grow()->getMorph_within_ratio() < 0)
        {
            line.clear();
            str_list = pop->Grow()->getMorphDistStDev();
            chars += c_file->write_vector(str_list, 2, QString(QString(" # Platoon_SD_Ratio")));
        }

        //ageing error parameters
        line = QString("#  Age Error from parameters");
        chars += c_file->writeline(line);
        if (data->get_age_composition()->getUseParameters())
        {
            for (i = 0; i < 7; i++)
            {
                line.clear();
                str_list = data->get_age_composition()->getErrorParam(i);
                chars += c_file->write_vector(str_list, 2, QString(QString(" #_AgeKeyParm%1").arg(QString::number(i+1))));
/*                for (int l = 0; l < str_list.count(); l++)
                    line.append(QString(" %1").arg(str_list.at(l)));
                line.append(QString(" #_AgeKeyParm%1").arg(QString::number(i+1)));
                chars += c_file->writeline (line);*/
            }
        }

        line = QString("# catch multiplier");
        chars += c_file->writeline(line);
        for (i = 0; i < data->get_num_fleets(); i++)
        {
            Fleet *fleet = data->getFleet(i);
            line.clear();
            if (fleet->getCatchMultiplier() > 0)
            {
                line = QString(QString("%1(%2)").arg(fleet->getName(), fleet->getNumber()));
                c_file->write_vector(fleet->getCatchMultParam(), 6, line);
            }
        }

        // Fraction Female
        line = QString("#  fraction female, by GP");
        chars += c_file->writeline(line);
        num = pop->Grow()->getNum_patterns();
        for (i = 0; i < num; i++)
        {
            QString hdr (QString("FracFemale_GP_%1").arg(QString::number(i + 1)));
            line.clear();
            str_list = pop->Grow()->getPattern(i)->getFractionFemaleParam();
            chars += c_file->write_vector(str_list, 2, hdr);
/*            for (int l = 0; l < str_list.count(); l++)
                line.append(QString(" %1").arg(str_list.at(l)));
            line.append(QString(" #_FracFemale_GP_%1").arg(QString::number(i + 1)));
            chars += c_file->writeline (line);*/
        }

        // time varying MG parameters
        chars += c_file->writeline ("#");
        line = QString ("# timevary MG parameters ");
        chars += c_file->writeline (line);
        line = QString("# LO HI INIT PRIOR PR_SD PR_type PHASE");
        chars += c_file->writeline (line);
        temp_int = 0;
        bool tVParms = false;
        if (pop->Grow()->getTimeVaryReadParams())
        {
        tablemodel *parmmodel;
        int num = 0;

        QStringList param;
        QString headr;
        growthPattern * gp;
        for (i = 0; i < pop->Grow()->getNum_patterns(); i++)
        {
            gp = pop->Grow()->getPattern(i);

            // female fish
            parmmodel = gp->getFemNatMTVParams();
            num = gp->getFemNumNatMTVParams();//parmmodel->getNumParamVars();
            tVParms = tVParms || (num > 0);
            for (int j = 0; j < num; j++)
            {
                param = gp->getFemNatMTVParam(j);//parmmodel->getParamVarData(natmIndex);
                headr = gp->getFemNatMTVParams()->getRowHeader(j);//parmmodel->getParamVarHeader(natmIndex);
                chars += c_file->write_vector(param, 2, headr);
            }
            parmmodel = gp->getFemGrowthTVParams();//getGrowthParamModel();
            num = parmmodel->rowCount();
            tVParms = tVParms || (num > 0);
            for (int j = 0; j < num; j++)
            {
                param = parmmodel->getRowData(j);
                headr = parmmodel->getRowHeader(j);
                chars += c_file->write_vector(param, 2, headr);
            }
            parmmodel = gp->getFemCVTVParams();
            num = parmmodel->rowCount();
            tVParms = tVParms || (num > 0);
            for (int j = 0; j < num; j++)
            {
                param = parmmodel->getRowData(j);
                headr = parmmodel->getRowHeader(j);
                chars += c_file->write_vector(param, 2, headr);
            }
            parmmodel = gp->getFemWtLenTVParams();
            num = parmmodel->rowCount();
            tVParms = tVParms || (num > 0);
            for (int j = 0; j < num; j++)
            {
                param = parmmodel->getRowData(j);
                headr = parmmodel->getRowHeader(j);
                chars += c_file->write_vector(param, 2, headr);
            }
            parmmodel = gp->getFemMatureTVParams();
            num = parmmodel->rowCount();
            tVParms = tVParms || (num > 0);
            for (int j = 0; j < num; j++)
            {
                param = parmmodel->getRowData(j);
                headr = parmmodel->getRowHeader(j);
                chars += c_file->write_vector(param, 2, headr);
            }
        }
        if (data->get_num_genders() > 1)
        {
            for (i = 0; i < pop->Grow()->getNum_patterns(); i++)
            {
                gp = pop->Grow()->getPattern(i);
                // male fish
                parmmodel = gp->getMaleNatMTVParams();
                num = parmmodel->rowCount();
                tVParms = tVParms || (num > 0);
                for (int j = 0; j < num; j++)
                {
                    param = parmmodel->getRowData(j);
                    headr = parmmodel->getRowHeader(j);
                    chars += c_file->write_vector(param, 2, headr);
                }

                parmmodel = gp->getMaleGrowthTVParams();
                num = parmmodel->rowCount();
                tVParms = tVParms || (num > 0);
                for (int j = 0; j < num; j++)
                {
                    param = parmmodel->getRowData(j);
                    headr = parmmodel->getRowHeader(j);
                    chars += c_file->write_vector(param, 2, headr);
                }

                parmmodel = gp->getMaleCVTVParams();
                num = parmmodel->rowCount();
                tVParms = tVParms || (num > 0);
                for (int j = 0; j < num; j++)
                {
                    param = parmmodel->getRowData(j);
                    headr = parmmodel->getRowHeader(j);
                    chars += c_file->write_vector(param, 2, headr);
                }
                parmmodel = gp->getMaleWtLenTVParams();
                num = parmmodel->rowCount();
                tVParms = tVParms || (num > 0);
                for (int j = 0; j < num; j++)
                {
                    param = parmmodel->getRowData(j);
                    headr = parmmodel->getRowHeader(j);
                    chars += c_file->write_vector(param, 2, headr);
                }
            }
        }
        for (i = 0; i < pop->Grow()->getNum_patterns(); i++)
        {
            gp = pop->Grow()->getPattern(i);
            num = gp->getNumHermaphTVParams();
            parmmodel = gp->getHermaphTVParams();
            tVParms = tVParms || (num > 0);
            for (int j = 0; j < num; j++)
            {
                param = parmmodel->getRowData(j);
                headr = parmmodel->getRowHeader(j);
                chars += c_file->write_vector(param, 2, headr);
            }

        }
        num = pop->SR()->getNumDistTVParams();
        parmmodel = pop->SR()->getDistTVParams();
        tVParms = tVParms || (num > 0);
        for (i = 0; i < num; i++)
        {
            param = parmmodel->getRowData(i);
            headr = parmmodel->getRowHeader(i);
            chars += c_file->write_vector(param, 2, headr);
        }
        num = pop->Grow()->getNumCohortTVParams();
        parmmodel = pop->Grow()->getCohortTVParams();
        tVParms = tVParms || (num > 0);
        for (i = 0; i < num; i++)
        {
            param = parmmodel->getRowData(i);
            headr = parmmodel->getRowHeader(i);
            chars += c_file->write_vector(param, 2, headr);
        }
        num = pop->Move()->getNumTVParams();
        parmmodel = pop->Move()->getMoveTVParams();
        tVParms = tVParms || (num > 0);
        for (i = 0; i < num; i++)
        {
            param = parmmodel->getRowData(i);
            headr = parmmodel->getRowHeader(i);
            chars += c_file->write_vector(param, 2, headr);
        }
        for (i = 0; i < pop->Grow()->getNum_patterns(); i++)
        {
            gp = pop->Grow()->getPattern(i);
            num = gp->getNumFracFmTVParams();
            parmmodel = gp->getFracFmTVParams();
            tVParms = tVParms || (num > 0);
            for (int j = 0; j < num; j++)
            {
                param = parmmodel->getRowData(j);
                headr = parmmodel->getRowHeader(j);
                chars += c_file->write_vector(param, 2, headr);
            }
        }
        }
        if (!tVParms)
        {
            line = QString("#_Cond -2 2 0 0 -1 99 -2 #_placeholder when no time-vary parameters");
            chars += c_file->writeline (line);
        }
//        line = QString ("# info on dev vectors created for MGparms are reported with other devs after tag parameter section ");
//        chars += c_file->writeline (line);
        chars += c_file->writeline ("#");

        line = QString("#_seasonal_effects_on_biology_parms");
        chars += c_file->writeline(line);
        line.clear();
        str_list = pop->getSeasParamSetup();
        chars += c_file->write (QByteArray(" "));
        chars += c_file->write_vector(str_list, 2, "femwtlen1,femwtlen2,mat1,mat2,fec1,fec2,Malewtlen1,malewtlen2,L1,K");

        line = QString("#_ LO HI INIT PRIOR PR_SD PR_type PHASE");
        chars += c_file->writeline(line);
        num = pop->getNumSeasParams();
        if (num == 0)
        {
            line = QString("#_Cond -2 2 0 0 -1 99 -2 #_placeholder when no seasonal MG parameters");
            chars += c_file->writeline(line);
        }
        else
        {
            for (i = 0; i < num; i++)
            {
                line.clear();
                str_list = pop->getSeasonalParam(i);
                chars += c_file->write_vector(str_list, 2, QString("MG-environ param %1").arg(QString::number(i)));
            }
        }
        chars += c_file->writeline ("#");

        // Spawner-recruitment
        temp_int = pop->SR()->getMethod();
        chars += c_file->write_val(temp_int, 1, QString("SR_function: 2=Ricker; 3=std_B-H; 4=SCAA; 5=Hockey; 6=B-H_flattop; 7=survival_3Parm; 8=Shepard_3Parm"));
        temp_int = pop->SR()->getUseSteepness();
        chars += c_file->write_val(temp_int, 1, QString("0/1 to use steepness in initial equ recruitment calculation"));
        temp_int = pop->SR()->getFeature();
        chars += c_file->write_val(temp_int, 1, QString("future feature:  0/1 to make realized sigmaR a function of SR curvature"));

        line = QString("#_ LO HI INIT PRIOR PR_SD PR_type PHASE env-var use_dev dev_mnyr dev_mxyr dev_PH Block Blk_Fxn  #_ parm_name");
        chars += c_file->writeline(line);

        num = 0;
        {
        tablemodel *params = pop->SR()->getFullParameters();
        num = params->rowCount();
        for (j = 0; j < num; j++)
        {
            chars += c_file->write_vector(params->getRowData(j), 4, params->getRowHeader(j));
        }
        }
        if (pop->SR()->getTimeVaryReadParams())
        {
            timeVaryParameterModel *params = pop->SR()->getTVParameterModel();
            num = params->getNumVarParams();
            if (num > 0)
            {
                line = QString("#Next are short parm lines for timevary ");
                chars += c_file->writeline(line);
//                line = QString("#_ LO HI INIT PRIOR PR_SD PR_type PHASE  #_ parm_name");
//                chars += c_file->writeline(line);
                for (j = 0; j < num; j++)
                {
                    chars += c_file->write_vector(params->getVarParameter(j), 4, params->getVarParamHeader(j));
                }
            }
            else
            {
                line = QString("#_No time-vary recruitment params");
                chars += c_file->writeline(line);
            }
        }
/*        line = QString(QString ("%1 #_").arg(pop->SR()->full_parameters->getRowText(num)));
        line.append(pop->SR()->full_parameters->getRowHeader(num++));
        chars += c_file->writeline(line);
        line = QString(QString ("%1 #_").arg(pop->SR()->full_parameters->getRowText(num)));
        line.append(pop->SR()->full_parameters->getRowHeader(num++));
        chars += c_file->writeline(line);
        if (pop->SR()->method == 5 ||
                pop->SR()->method == 7 ||
                pop->SR()->method == 8)
        {
            line = QString(QString ("%1 #_").arg(pop->SR()->full_parameters->getRowText(num)));
            line.append(pop->SR()->full_parameters->getRowHeader(num++));
            chars += c_file->writeline(line);
        }
        line = QString(QString ("%1 #_").arg(pop->SR()->full_parameters->getRowText(num)));
        line.append(pop->SR()->full_parameters->getRowHeader(num++));
        chars += c_file->writeline(line);
        line = QString(QString ("%1 #_").arg(pop->SR()->full_parameters->getRowText(num)));
        line.append(pop->SR()->full_parameters->getRowHeader(num++));
        chars += c_file->writeline(line);
        line = QString(QString ("%1 #_").arg(pop->SR()->full_parameters->getRowText(num)));
        chars += c_file->writeline(line);
        line.append(pop->SR()->full_parameters->getRowHeader(num++));

        line = QString("#Next are short parm lines, if requested, for env effects on R0, steepness, and annual dev");
        chars += c_file->writeline(line);
        line = QString("#Then short parm lines, if requested, for block/trend effects on R0, steepness, and annual dev");
        chars += c_file->writeline(line);*/

        chars += c_file->write_val(pop->SR()->getRecDevCode(), 1,
                                   QString("do_recdev:  0=none; 1=devvector (R=F(SSB)+dev); 2=deviations (R=F(SSB)+dev); 3=deviations (R=R0*dev; dev2=R-f(SSB)); 4=like 3 with sum(dev2) adding penalty"));
        chars += c_file->write_val(pop->SR()->getRecDevStartYr(), 1,
                                   QString("first year of main recr_devs; early devs can preceed this era"));
        chars += c_file->write_val(pop->SR()->getRecDevEndYr(), 1,
                                   QString ("last year of main recr_devs; forecast devs start in following year"));
        chars += c_file->write_val(pop->SR()->getRecDevPhase(), 1,
                                   QString ("recdev phase "));
        chars += c_file->write_val(pop->SR()->getAdvancedOpts()? "1":"0", 1,
                                   QString ("(0/1) to read 13 advanced options"));

        if (pop->SR()->getAdvancedOpts())
        {
            temp_int = pop->SR()->getRecDevEarlyStart();
            chars += c_file->write_val(temp_int, 1, QString("recdev_early_start (0=none; neg value makes relative to recdev_start)"));
            temp_int = pop->SR()->getRecDevEarlyPhase();
            chars += c_file->write_val(temp_int, 1, QString("recdev_early_phase"));
            temp_int = pop->SR()->getFcastRecPhase();
            chars += c_file->write_val(temp_int, 1, QString("forecast_recruitment phase (incl. late recr) (0 value resets to maxphase+1)"));
            temp_doub = pop->SR()->getFcastLambda();
            chars += c_file->write_val(temp_doub, 1, QString("lambda for Fcast_recr_like occurring before endyr+1"));
            temp_doub = pop->SR()->getNobiasLastEarlyYr();
            chars += c_file->write_val(temp_doub, 1, QString("last_yr_nobias_adj_in_MPD; begin of ramp"));
            temp_doub = pop->SR()->getFullbiasFirstYr();
            chars += c_file->write_val(temp_doub, 1, QString("first_yr_fullbias_adj_in_MPD; begin of plateau"));
            temp_doub = pop->SR()->getFullbiasLastYr();
            chars += c_file->write_val(temp_doub, 1, QString("last_yr_fullbias_adj_in_MPD"));
            temp_doub = pop->SR()->getNobiasFirstRecentYr();
            chars += c_file->write_val(temp_doub, 1, QString("end_yr_for_ramp_in_MPD (can be in forecast to shape ramp, but SS sets bias_adj to 0.0 for fcast yrs)"));
            temp_doub = pop->SR()->getMaxBiasAdjust();
            chars += c_file->write_val(temp_doub, 1, QString("max_bias_adj_in_MPD (typical ~0.8; -3 sets all years to 0.0; -2 sets all non-forecast yrs w/ estimated recdevs to 1.0; -1 sets biasadj=1.0 for all yrs w/ recdevs)"));
            temp_int = pop->SR()->getRecCycles();
            chars += c_file->write_val(temp_int, 1, QString("period of cycles in recruitment (N parms read below)"));
            temp_doub = pop->SR()->getRecDevMin();
            chars += c_file->write_val(temp_doub, 1, QString("min rec_dev"));
            temp_doub = pop->SR()->getRecDevMax();
            chars += c_file->write_val(temp_doub, 1, QString ("max rec_dev"));
            temp_int = pop->SR()->getNumRecDev();
            chars += c_file->write_val(temp_int, 1, QString ("read recdevs"));
            chars += c_file->writeline(QString ("#_end of advanced SR options"));
        }
        line = QString("#");
        chars += c_file->writeline(line);

        if (pop->SR()->getRecCycles() == 0)
        {
            line = QString("#_placeholder for full parameter lines for recruitment cycles");
            chars += c_file->writeline(line);
        }
        else
        {
            for (i = 0; i < pop->SR()->getRecCycles(); i++)
            {
                line.clear();
                str_list = pop->SR()->getCycleParam(i);
                for (int j = 0; j < 14; j++)
                    line.append(QString(" %1").arg(str_list.at(j)));
                line.append(QString (" #_"));
                chars += c_file->writeline(line);
            }
        }

        line = QString("#_read specified recr devs");
        chars += c_file->writeline(line);
        line = QString("#_Yr Input_value");
        chars += c_file->writeline(line);
        for (i = 0; i < pop->SR()->getNumRecDev(); i++)
        {
            str_list = pop->SR()->getRecruitDev(i);
            chars += c_file->write_vector(str_list, 4);
        }

        line = QString("#");
        chars += c_file->writeline(line);
        c_file->newline();

        // mortality
        line = QString("#Fishing Mortality info ");
        chars += c_file->writeline(line);
        chars += c_file->write_val(pop->M()->getBparkF(), 1, QString("F ballpark value in units of annual_F"));
        chars += c_file->write_val(pop->M()->getBparkYr(), 1, QString("F ballpark year (neg value to disable)"));
        chars += c_file->write_val(pop->M()->getMethod(), 1, QString ("F_Method:  1=Pope midseason rate; 2=F as parameter; 3=F as hybrid; 4=fleet-specific parm/hybrid (#4 is superset of #2 and #3 and is recommended)"));
        chars += c_file->write_val(pop->M()->getMaxF(), 1, QString("max F (methods 2-4) or harvest fraction (method 1)"));
        line = QString("# for Fmethod=1; no additional F input needed");
        chars += c_file->writeline(line);
        line = QString("# for Fmethod=2; read overall start F value; overall phase; N detailed inputs to read");
        chars += c_file->writeline(line);
        line = QString("# for Fmethod=3; read N iterations for tuning");
        chars += c_file->writeline(line);
        line = QString("# for Fmethod=4; read list of fleets needing parameters; syntax is: fleet, F_starting_value (if start_PH=1), first PH for parms (99 to stay in hybrid)");
        chars += c_file->writeline(line);
        line = QString("#                then read N tuning loops for hybrid fleets 2-3 normally enough");
        chars += c_file->writeline(line);

        switch (pop->M()->getMethod())
        {
        case 1:
            break;

        case 2:
            chars += c_file->write_val(pop->M()->getStartF(), 1,
                       QString ("overall start F value (all fleets; used if start phase = 1 and not reading parfile)"));
            chars += c_file->write_val(pop->M()->getPhase(), 1,
                       QString ("start phase for parms (does hybrid in early phases)"));
            chars += c_file->write_val(pop->M()->getNumInputs(), 1,
                       QString ("N detailed inputs to read"));
            break;

        case 3:
            chars += c_file->write_val(pop->M()->getNumTuningIters(), 1,
                       QString ("N iterations for tuning in hybrid mode; recommend 3 (faster) to 5 (more precise if many fleets)"));
            break;

        case 4:
            line = QString("#Fleet start_F first_parm_phase");
            chars += c_file->writeline(line);
            temp_int = pop->M()->getNumFleetSpecF();
            for (int i = 0; i < temp_int; i++)
            {
                c_file->write_vector(pop->M()->getFleetSpecF(i), 2);
            }
            line = QString("-9999 1 1");
            chars += c_file->writeline(line);
            chars += c_file->write_val(pop->M()->getNumTuningIters(), 1,
                       QString ("%1 #_number of tuning loops for hybrid fleets; 4 good; 3 faster"));
            break;

        }

        line = QString ("#_Fleet Yr Seas F_value se phase (for detailed setup of F_Method=2; -Yr to fill remaining years)");
        chars += c_file->writeline(line);
        temp_int = pop->M()->getNumInputs();
        for (i = 0; i < temp_int; i++)
        {
            num = pop->M()->getInputLine(i).at(i).toInt();
            chars += c_file->write_vector(pop->M()->getInputLine(i), 5,
                       data->getFleet(num)->getName());
        }
        chars += c_file->writeline("#");

        num = pop->M()->getNumInitialParams();
        line = QString(QString("#_initial_F_parms; for each fleet x season that has init_catch; nest season in fleet; count = %1").arg (QString::number(num)));
        chars += c_file->writeline(line);
        line = QString("#_for unconstrained init_F, use an arbitrary initial catch and set lambda=0 for its logL");
        chars += c_file->writeline(line);
        line = QString("#_ LO HI INIT PRIOR PR_SD PR_type PHASE");
        chars += c_file->writeline(line);
        for (i = 0; i < num; i++)
        {
            str_list = pop->M()->getInitialParam(i);
            temp_string = pop->M()->getInitialParams()->getRowHeader(i);
            temp_int = temp_string.section('t', 1, -1).toInt();
            chars += c_file->write_vector(str_list, 4,
                                          QString("InitF_seas_%1_flt_%2%3").arg
                                          (QString::number(1),
                                           QString::number(temp_int),
                                           data->getFleet(temp_int-1)->getName()));
        }
        line = QString("#_");
        chars += c_file->writeline(line);
        c_file->newline();

        // Q_setup
        chars += c_file->writeline(QString("#_Q_setup for fleets with cpue or survey data"));
        chars += c_file->writeline(QString("#_1:  fleet number"));
        chars += c_file->writeline(QString("#_2:  link type: (1=simple q, 1 parm; 2=mirror simple q, 1 mirrored parm; 3=q and power, 2 parm)"));
        chars += c_file->writeline(QString("#_3:  extra input for link, i.e. mirror fleet"));
        chars += c_file->writeline(QString("#_4:  0/1 to select extra sd parameter"));
        chars += c_file->writeline(QString("#_5:  0/1 for biasadj or not"));
        chars += c_file->writeline(QString("#_6:  0/1 to float"));
        chars += c_file->writeline(QString("#_survey: 7 Depletion is a depletion fleet"));
        chars += c_file->writeline(QString("#_Q_setup(f,2)=0; add 1 to phases of all parms; only R0 active in new phase 1"));
        chars += c_file->writeline(QString("#_fleet   link link_info extra_se biasadj float #_fleetname"));
        for (int i = 0; i < data->get_num_fleets(); i++)
        {
            if (data->getFleet(i)->getQSetupRead())
            {
                line = QString(QString("      %1").arg(i + 1));
                line.append(data->getFleet(i)->Q()->getSetup());
                line.append(QString("  #_ %1").arg(
                                data->getFleet(i)->getName()));
                chars += c_file->writeline(line);
            }
        }
        chars += c_file->writeline(QString ("  -9999      0      0      0      0      0  #_ terminator"));
        chars += c_file->writeline(QString ("#"));
        // Q_parameters
        line = QString("#_Q_parms(if_any);Qunits_are_ln(q)");
        chars += c_file->writeline(line);
        line = QString("#_LO      HI      INIT    PRIOR   PR_SD   PR_type  PHASE   env-var  use_dev  dev_mnyr dev_mxyr dev_PH   Block   Blk_Fxn #_parm_name");
        chars += c_file->writeline(line);
        num = data->get_num_fleets();
        for (int i = 0; i < num; i++)
        {
            Fleet *fleet = data->getFleet(i);
            // Q Base
            if (fleet->getQSetupRead())
            {
                line = QString(QString("LnQ_base_%2(%3)").arg(
                                fleet->getName(),
                                QString::number(i+1)));
                chars += c_file->write_vector(fleet->Q()->getLinkParam(), 8, line);
                // Q Mirror Offset
                if (fleet->Q()->getDoMirOffset())
                {
                    line = QString(QString("Q_Mirror_Offset_%2(%3)").arg(
                                    fleet->getName(),
                                    QString::number(i+1)));
                    chars += c_file->write_vector(fleet->Q()->getMirOffsetParam(), 8, line);
                }
                // Q Power
                if (fleet->Q()->getDoPower())
                {
                    line = QString(QString("Q_power_%2(%3)").arg(
                                    fleet->getName(),
                                    QString::number(i+1)));
                    chars += c_file->write_vector(fleet->Q()->getPowerParam(), 8, line);
                }
                // Q Extra SD
                if (fleet->Q()->getDoExtraSD())
                {
                    line = QString(QString("Q_extraSD_%2(%3)").arg(
                                    fleet->getName(),
                                    QString::number(i+1)));
                    chars += c_file->write_vector(fleet->Q()->getExtraParam(), 8, line);
                }
            }
        }

        // Q_timevary
        temp_int = data->getFleet(0)->getQTimeVaryReadParams();
        if (temp_int > 0)
        {
            for (i = 0; i < data->get_num_fleets(); i++)
                temp_int += data->getFleet(i)->Q()->getNumTimeVaryParams();
            if (temp_int > 0)
            {
                line = QString("# timevary Q parameters");
                chars += c_file->writeline(line);
                line = QString("#       LO        HI      INIT     PRIOR   PR_SD  PR_type      PHASE");
                chars += c_file->writeline(line);
                for (i = 0; i < data->get_num_fleets(); i++)
                {
                    q_ratio *fleetQ = data->getFleet(i)->Q();
                    int num = fleetQ->getNumTimeVaryParams();
                    for (int j = 0; j < num; j++)
                    {
                        chars += c_file->write_vector(fleetQ->getTVParam(j), 7,
                                                      fleetQ->getTimeVaryParamLabel(j));
                    }
                }
            }
        }
        if (temp_int == 0)
        {
            line = QString("# no timevary Q parameters");
            chars += c_file->writeline(line);
        }

        line = QString("# info on dev vectors created for Q parms are reported with other devs after tag parameter section ");
        chars += c_file->writeline(line);

        // Selectivity
        chars += c_file->writeline(QString("#"));
        chars += c_file->writeline(QString("#_size_selex_patterns"));
        chars += c_file->writeline(QString("#Pattern:_0;  parm=0; selex=1.0 for all sizes"));
        chars += c_file->writeline(QString("#Pattern:_1;  parm=2; logistic; with 95% width specification"));
        chars += c_file->writeline(QString("#Pattern:_5;  parm=2; mirror another size selex; PARMS pick the min-max bin to mirror"));
        chars += c_file->writeline(QString("#Pattern:_11; parm=2; selex=1.0  for specified min-max population length bin range"));
        chars += c_file->writeline(QString("#Pattern:_15; parm=0; mirror another age or length selex"));
        chars += c_file->writeline(QString("#Pattern:_6;  parm=2+special; non-parm len selex"));
        chars += c_file->writeline(QString("#Pattern:_43; parm=2+special+2;  like 6, with 2 additional param for scaling (average over bin range)"));
        chars += c_file->writeline(QString("#Pattern:_8;  parm=8; double_logistic with smooth transitions and constant above Linf option"));
        chars += c_file->writeline(QString("#Pattern:_9;  parm=6; simple 4-parm double logistic with starting length; parm 5 is first length; parm 6=1 does desc as offset"));
        chars += c_file->writeline(QString("#Pattern:_21; parm=2+special; non-parm len selex, read as pairs of size, then selex"));
        chars += c_file->writeline(QString("#Pattern:_22; parm=4; double_normal as in CASAL"));
        chars += c_file->writeline(QString("#Pattern:_23; parm=6; double_normal where final value is directly equal to sp(6) so can be >1.0"));
        chars += c_file->writeline(QString("#Pattern:_24; parm=6; double_normal with sel(minL) and sel(maxL), using joiners"));
        chars += c_file->writeline(QString("#Pattern:_2;  parm=6; double_normal with sel(minL) and sel(maxL), using joiners, back compatibile version of 24 with 3.30.18 and older"));
        chars += c_file->writeline(QString("#Pattern:_25; parm=3; exponential-logistic in length"));
        chars += c_file->writeline(QString("#Pattern:_27; parm=special+3; cubic spline in length; parm1==1 resets knots; parm1==2 resets all "));
        chars += c_file->writeline(QString("#Pattern:_42; parm=special+3+2; cubic spline; like 27, with 2 additional param for scaling (average over bin range)"));
        chars += c_file->writeline(QString("#_discard_options:_0=none;_1=define_retention;_2=retention&mortality;_3=all_discarded_dead;_4=define_dome-shaped_retention"));
        chars += c_file->writeline(QString("#_Pattern Discard Male Special"));
        for (int i = 0; i < data->get_num_fleets(); i++)
        {
            chars += c_file->write_vector(data->getFleet(i)->getSizeSelectivity()->getSetup(),
                                 3, QString("%1 %2").arg(QString::number(i + 1),
                                                         data->getFleet(i)->getName()));
        }

        chars += c_file->writeline(QString("#"));
        chars += c_file->writeline(QString("#_age_selex_patterns"));
        chars += c_file->writeline(QString("#Pattern:_0;  parm=0; selex=1.0 for ages 0 to maxage"));
        chars += c_file->writeline(QString("#Pattern:_10; parm=0; selex=1.0 for ages 1 to maxage"));
        chars += c_file->writeline(QString("#Pattern:_11; parm=2; selex=1.0  for specified min-max age"));
        chars += c_file->writeline(QString("#Pattern:_12; parm=2; age logistic"));
        chars += c_file->writeline(QString("#Pattern:_13; parm=8; age double logistic. Recommend using pattern 18 instead."));
        chars += c_file->writeline(QString("#Pattern:_14; parm=nages+1; age empirical"));
        chars += c_file->writeline(QString("#Pattern:_15; parm=0; mirror another age or length selex"));
        chars += c_file->writeline(QString("#Pattern:_16; parm=2; Coleraine - Gaussian"));
        chars += c_file->writeline(QString("#Pattern:_17; parm=nages+1; empirical as random walk  N parameters to read can be overridden by setting special to non-zero"));
        chars += c_file->writeline(QString("#Pattern:_41; parm=2+nages+1; // like 17, with 2 additional param for scaling (average over bin range)"));
        chars += c_file->writeline(QString("#Pattern:_18; parm=8; double logistic - smooth transition"));
        chars += c_file->writeline(QString("#Pattern:_19; parm=6; simple 4-parm double logistic with starting age"));
        chars += c_file->writeline(QString("#Pattern:_20; parm=6; double_normal,using joiners"));
        chars += c_file->writeline(QString("#Pattern:_26; parm=3; exponential-logistic in age"));
        chars += c_file->writeline(QString("#Pattern:_27; parm=3+special; cubic spline in age; parm1==1 resets knots; parm1==2 resets all "));
        chars += c_file->writeline(QString("#Pattern:_42; parm=2+special+3; // cubic spline; with 2 additional param for scaling (average over bin range)"));
        chars += c_file->writeline(QString("#Age patterns entered with value >100 create Min_selage from first digit and pattern from remainder"));
        chars += c_file->writeline(QString("#_Pattern Discard Male Special"));
        for (int i = 0; i < data->get_num_fleets(); i++)
        {
            str_list = data->getFleet(i)->getAgeSelectivity()->getSetup();
            if (data->getFleet(i)->getAgeSelectivity()->getMinSel() > 0) {
                int selopt = str_list.takeFirst().toInt();
                selopt += data->getFleet(i)->getAgeSelectivity()->getMinSel();
                str_list.prepend(QString::number(selopt));
            }
            chars += c_file->write_vector(str_list, 3, QString("%1 %2").arg(QString::number(i + 1),
                                                         data->getFleet(i)->getName()));
        }
        chars += c_file->writeline(QString("#"));
        chars += c_file->writeline(QString ("#_ LO   HI   INIT  PRIOR PR_SD PR_type PHASE env-var use_dev dev_mnyr dev_mxyr dev_PH Block Blk_Fxn #_parm_name"));
        for (int i = 0; i < data->get_num_fleets(); i++)
        {
            Fleet *fleet = data->getFleet(i);
            line = QString("# %1   %2 LenSelex").arg(QString::number(fleet->getNumber()), fleet->getName());
            chars += c_file->writeline(line);
            selectivity *slx = fleet->getSizeSelectivity();
            num = slx->getNumParameters();
            for (int j = 0; j < num; j++)
            {
                chars += c_file->write_vector(slx->getParameter(j), 6, slx->getParameterLabel(j));
            }
            for (int j = 0; j < slx->getNumRetainParameters(); j++)
            {
                chars += c_file->write_vector(slx->getRetainParameter(j), 6, slx->getRetainParameterLabel(j));
            }
            for (int j = 0; j < slx->getNumDiscardParameters(); j++)
            {
                chars += c_file->write_vector(slx->getDiscardParameter(j), 6, slx->getDiscardParameterLabel(j));
            }
            for (int j = 0; j < slx->getNumMaleParameters(); j++)
            {
                chars += c_file->write_vector(slx->getMaleParameter(j), 6, slx->getMaleParameterLabel(j));
            }
        }

        for (int i = 0; i < data->get_num_fleets(); i++)
        {
            Fleet *fleet = data->getFleet(i);
            line = QString("# %1   %2 AgeSelex").arg(QString::number(fleet->getNumber()), fleet->getName());
            chars += c_file->writeline(line);
            selectivity *slx = fleet->getAgeSelectivity();
            num = slx->getNumParameters();
            for (int j = 0; j < num; j++)
            {
                chars += c_file->write_vector(slx->getParameter(j), 6, slx->getParameterLabel(j));
            }
            for (int j = 0; j < slx->getNumRetainParameters(); j++)
            {
                chars += c_file->write_vector(slx->getRetainParameter(j), 6, slx->getRetainParameterLabel(j));
            }
            for (int j = 0; j < slx->getNumDiscardParameters(); j++)
            {
                chars += c_file->write_vector(slx->getDiscardParameter(j), 6, slx->getDiscardParameterLabel(j));
            }
            for (int j = 0; j < slx->getNumMaleParameters(); j++)
            {
                chars += c_file->write_vector(slx->getMaleParameter(j), 6, slx->getMaleParameterLabel(j));
            }
        }

        // Dirichlet Mult parameter
        ssComposition *comp = data->get_length_composition();
        int tot = 0;
        num_vals = comp->getNumDirichletParams();
        if (num_vals > 0)
            chars += c_file->writeline("# Dirichlet parameters");
        for (i = 0; i < num_vals; i++)
        {
            tot++;
            chars += c_file->write_vector(comp->getDirichletParam(i), 6, QString("ln(EffN mult) Length %1 Dirichlet").arg(i+1));
        }
        comp = data->get_age_composition();
        num_vals = comp->getNumDirichletParams();
        if (tot == 0 && num_vals > 0)
            chars += c_file->writeline("# Dirichlet parameters");
        for (i = 0; i < num_vals; i++)
        {
            tot++;
            chars += c_file->write_vector(comp->getDirichletParam(i), 6, QString("ln(EffN mult) Age %1 Dirichlet").arg(i+1));
        }
        if (tot == 0)
            chars += c_file->writeline(QString("#_No_Dirichlet parameters"));

        // Selex time varying params
        chars += c_file->writeline(QString("# timevary selex parameters"));
        line = QString(QString("#_   LO       HI     INIT    PRIOR    PR_SD    PR_type  PHASE"));
        chars += c_file->writeline(line);
        if (data->getFleet(0)->getSelTimeVaryReadParams())
        {
        for (i = 0; i < data->get_num_fleets(); i++)
        { // size selectivity
            selectivity *slx = data->getFleet(i)->getSizeSelectivity();
            num = slx->getNumTimeVaryParameters();
            for (int j = 0; j < num; j++)
            {
                chars += c_file->write_vector(slx->getTimeVaryParameter(j), 8, slx->getTimeVaryParameterLabel(j));
            }
            num = slx->getNumRetainTimeVaryParameters();
            for (int j = 0; j < num; j++)
            {
                chars += c_file->write_vector(slx->getRetainTimeVaryParameter(j), 8, slx->getRetainTimeVaryParameterLabel(j));
            }
            num = slx->getNumDiscardTimeVaryParameters();
            for (int j = 0; j < num; j++)
            {
                chars += c_file->write_vector(slx->getDiscardTimeVaryParameter(j), 8, slx->getDiscardTimeVaryParameterLabel(j));
            }
            num = slx->getNumMaleTimeVaryParameters();
            for (int j = 0; j < num; j++)
            {
                chars += c_file->write_vector(slx->getMaleTimeVaryParameter(j), 8, slx->getMaleTimeVaryParameterLabel(j));
            }
        }
        for (i = 0; i < data->get_num_fleets(); i++)
        { // age selectivity
            selectivity *slx = data->getFleet(i)->getAgeSelectivity();
            num = slx->getNumTimeVaryParameters();
            for (int j = 0; j < num; j++)
            {
                chars += c_file->write_vector(slx->getTimeVaryParameter(j), 8, slx->getTimeVaryParameterLabel(j));
            }
            num = slx->getNumRetainTimeVaryParameters();
            for (int j = 0; j < num; j++)
            {
                chars += c_file->write_vector(slx->getRetainTimeVaryParameter(j), 8, slx->getRetainTimeVaryParameterLabel(j));
            }
            num = slx->getNumDiscardTimeVaryParameters();
            for (int j = 0; j < num; j++)
            {
                chars += c_file->write_vector(slx->getDiscardTimeVaryParameter(j), 8, slx->getDiscardTimeVaryParameterLabel(j));
            }
            num = slx->getNumMaleTimeVaryParameters();
            for (int j = 0; j < num; j++)
            {
                chars += c_file->write_vector(slx->getMaleTimeVaryParameter(j), 8, slx->getMaleTimeVaryParameterLabel(j));
            }
        }
        }

        line = QString(QString("# info on dev vectors created for selex parms are reported with other devs after tag parameter section "));
        chars += c_file->writeline(line);
        chars += c_file->writeline("#");

        // 2D-AR1 smoother
        temp_int = data->getUse2DAR1()? 1: 0;
        chars += c_file->write_val(temp_int, 0, QString("use 2D_AR1 selectivity(0/1)"));
        if (temp_int == 0) {
            chars += c_file->writeline(QString("#_no 2D_AR1 selex offset used"));
        }
        else {
            chars += c_file->writeline(QString("#_specifications for 2D_AR1 and associated parameters"));
            for (i = 0; i < data->getNumActiveFleets(); i++) {
                Fleet *fleet = data->getFleet(i);
                QString fleetstr = QString::number(fleet->getNumber());
                if (fleet->get2DAR1()->getUse()) {
                    chars += c_file->writeline(QString("#_specs:"));
                    chars += c_file->writeline(QString("#fleet, ymin,   ymax,   amin,   amax,   sig_amax, use_rho, len1/age2, devphase, before_range, after_range"));
                    str_list = fleet->get2DAR1()->getSpec();
                    str_list.prepend(fleetstr);
                    chars += c_file->write_vector(str_list, 7, QString("2d_AR specs for fleet: %1").arg(fleet->getName()));
                    chars += c_file->writeline(QString("#_  LO    HI      INIT    PRIOR   PR_SD   PR_type PHASE #_parm_name"));
                    str_list = fleet->get2DAR1()->getParam(0);
                    chars += c_file->write_vector(str_list, 7, QString("sigma_sel for fleet:  %1, age").arg(fleetstr));
                    str_list = fleet->get2DAR1()->getParam(1);
                    chars += c_file->write_vector(str_list, 7, QString("rho_year for fleet:  %1").arg(fleetstr));
                    str_list = fleet->get2DAR1()->getParam(2);
                    chars += c_file->write_vector(str_list, 7, QString("rho_age for fleet:  %1").arg(fleetstr));
                }
            }
            chars += c_file->writeline(QString("-9999  0 0 0 0 0 0 0 0 0 0 # terminator"));
        }
        chars += c_file->writeline(QString("#"));

        // Tag Recapture Parameters
        temp_int = data->getTagLoss();
        line = QString(QString("# Tag loss and Tag reporting parameters go next"));
        chars += c_file->writeline(line);
        line = QString(QString("%1 # TG_custom:  0=no read and autogen if tag data exist; 1=read ").arg(
                           QString::number(temp_int)));
        chars += c_file->writeline(line);
        if (temp_int == 1)
        {
            int numCfleets = 0;
            for (int i = 0; i < data->get_num_fleets(); i++)
                if (data->getFleet(i)->getType() == 1 ||
                        data->getFleet(i)->getType() == 2)
                    numCfleets++;
            num = data->getNumTagGroups();
            // tag loss init
            for (i = 0; i < num; i++) {
                str_list = data->getTagLossInit()->getParameter(i);
                chars += c_file->write_vector(str_list, 3, QString("TG_loss_init_%1").arg(QString::number(i+1)));
            }
            // tag loss chronic
            for (i = 0; i < num; i++) {
                str_list = data->getTagLossChronic()->getParameter(i);
                chars += c_file->write_vector(str_list, 3, QString("TG_loss_chronic_%1").arg(QString::number(i+1)));
            }
            // tag overdispersion
            for (i = 0; i < num; i++) {
                str_list = data->getTagOverdispersion()->getParameter(i);
                chars += c_file->write_vector(str_list, 3, QString("TG_overdispersion_%1").arg(QString::number(i+1)));
            }
            // tag report fleet
            for (i = 0; i < numCfleets; i++) {
                str_list = data->getTagReportFleet()->getParameter(i);
                chars += c_file->write_vector(str_list, 3, QString("TG_report_fleet:_%1").arg(QString::number(i+1)));
            }
            // tag report decay
            for (i = 0; i < numCfleets; i++) {
                str_list = data->getTagReportDecay()->getParameter(i);
                chars += c_file->write_vector(str_list, 3, QString("TG_rpt_decay_fleet:_%1").arg(QString::number(i+1)));
            }

//            data->setTagLossParameter(c_file->read_line());
            // tag time varying
            num = data->getTagLossInitTV()->getNumVarParams();
            for (i = 0; i < num; i++) {
                str_list = data->getTagLossInitTV()->getVarParameter(i);
                temp_string = data->getTagLossInitTV()->getVarParamHeader(i);
                chars += c_file->write_vector(str_list, 3, temp_string);
            }
            num = data->getTagLossChronicTV()->getNumVarParams();
            for (i = 0; i < num; i++) {
                str_list = data->getTagLossChronicTV()->getVarParameter(i);
                temp_string = data->getTagLossChronicTV()->getVarParamHeader(i);
                chars += c_file->write_vector(str_list, 3, temp_string);
            }
            num = data->getTagOverdispersionTV()->getNumVarParams();
            for (i = 0; i < num; i++) {
                str_list = data->getTagOverdispersionTV()->getVarParameter(i);
                temp_string = data->getTagOverdispersionTV()->getVarParamHeader(i);
                chars += c_file->write_vector(str_list, 3, temp_string);
            }
            num = data->getTagReportFleetTV()->getNumVarParams();
            for (i = 0; i < num; i++) {
                str_list = data->getTagReportFleetTV()->getVarParameter(i);
                temp_string = data->getTagReportFleetTV()->getVarParamHeader(i);
                chars += c_file->write_vector(str_list, 3, temp_string);
            }
            num = data->getTagReportDecayTV()->getNumVarParams();
            for (i = 0; i < num; i++) {
                str_list = data->getTagReportDecayTV()->getVarParameter(i);
                temp_string = data->getTagReportDecayTV()->getVarParamHeader(i);
                chars += c_file->write_vector(str_list, 3, temp_string);
            }
        }
        else
        {
            line = QString(QString("#_Cond -6 6 1 1 2 0.01 -4 0 0 0 0 0 0 0  #_placeholder if no parameters"));
            chars += c_file->writeline(line);
        }

        chars += c_file->writeline("#");
        temp_int = data->getInputValueVariance();
        chars += c_file->writeline(QString("# Input variance adjustments factors: "));
        chars += c_file->writeline(QString(" #_1=add_to_survey_CV"));
        chars += c_file->writeline(QString(" #_2=add_to_discard_stddev"));
        chars += c_file->writeline(QString(" #_3=add_to_bodywt_CV"));
        chars += c_file->writeline(QString(" #_4=mult_by_lencomp_N"));
        chars += c_file->writeline(QString(" #_5=mult_by_agecomp_N"));
        chars += c_file->writeline(QString(" #_6=mult_by_size-at-age_N"));
        chars += c_file->writeline(QString(" #_7=mult_by_generalized_sizecomp"));
        chars += c_file->writeline(QString("#_Factor  Fleet  Value"));
        for (int fl = 0; fl < data->get_num_fleets(); fl++)
        {
            for (int i = 1; i <= 7; i++)
            {
                if (data->getFleet(fl)->getDoInputVariance(i))
                {
                    temp_float = data->getFleet(fl)->getInputVarianceValue(i);
                    line = QString(QString("    %1    %2    %3 ").arg(
                                       QString::number(i),
                                       QString::number(fl+1),
                                       QString::number(temp_float)));
                    chars += c_file->writeline(line);
                }
            }
        }
/*        for (int i = 0; i < data->get_num_fleets(); i++)
        {
            temp_float = data->getFleet(i)->getAddToSurveyCV();
            if (!floatEquals(temp_float, 0.0))
            {
                line = QString(QString("    1    %1    %2  #_add_to_survey_CV").arg(
                                   QString::number(i+1),
                                   QString::number(temp_float)));
                chars += c_file->writeline(line);
            }
        }
        for (int i = 0; i < data->get_num_fleets(); i++)
        {
            temp_float = data->getFleet(i)->getAddToDiscardSD();
            if (!floatEquals(temp_float, 0.0))
            {
                line = QString(QString("    2    %1    %2  #_add_to_discard_stddev").arg(
                                   QString::number(i+1),
                                   QString::number(temp_float)));
                chars += c_file->writeline(line);
            }
            temp_float = data->getFleet(i)->getAddToBodyWtCV();
            if (!floatEquals(temp_float, 0.0))
            {
                line = QString(QString("    3    %1    %2  #_add_to_bodywt_CV").arg(
                                   QString::number(i+1),
                                   QString::number(temp_float)));
                chars += c_file->writeline(line);
            }
            temp_float = data->getFleet(i)->getMultByLenCompN();
            if (!floatEquals(temp_float, 0.0))
            {
                line = QString(QString("    4    %1    %2  #_mult_by_lencomp_N").arg(
                                   QString::number(i+1),
                                   QString::number(temp_float)));
                chars += c_file->writeline(line);
            }
            temp_float = data->getFleet(i)->getMultByAgeCompN();
            if (!floatEquals(temp_float, 0.0))
            {
                line = QString(QString("    5    %1    %2  #_mult_by_agecomp_N").arg(
                                   QString::number(i+1),
                                   QString::number(temp_float)));
                chars += c_file->writeline(line);
            }
            temp_float = data->getFleet(i)->getMultBySAA();
            if (!floatEquals(temp_float, 0.0))
            {
                line = QString(QString("    6    %1    %2  #_mult_by_size-at-age_N").arg(
                                   QString::number(i+1),
                                   QString::number(temp_float)));
                chars += c_file->writeline(line);
            }
            temp_float = data->getFleet(i)->getMultByGenSize();
            if (!floatEquals(temp_float, 0.0))
            {
                line = QString(QString("    7    %1    %2  #_mult_by_gen_sizecomp").arg(
                                   QString::number(i+1),
                                   QString::number(temp_float)));
                chars += c_file->writeline(line);
            }
        }*/
        line = QString (" -9999    1     0  # terminator");
        chars += c_file->writeline(line);


        chars += c_file->writeline("#");
        chars += c_file->write_val(data->getLambdaMaxPhase(), 1, QString("maxlambdaphase"));
        chars += c_file->write_val(data->getLambdaSdOffset(), 1, QString("sd_offset; must be 1 if any growthCV, sigmaR, or survey extraSD is an estimated parameter"));
        line = QString(QString("# read %1 changes to default Lambdas (default value is 1.0)").arg(QString::number(data->getNumLambdaAdjustments())));
        chars += c_file->writeline(line);
        chars += c_file->writeline(QString("# Like_comp codes:  1=surv; 2=disc; 3=mnwt; 4=length; 5=age; 6=SizeFreq; 7=sizeage; 8=catch; 9=init_equ_catch; "));
        chars += c_file->writeline(QString("# 10=recrdev; 11=parm_prior; 12=parm_dev; 13=CrashPen; 14=Morphcomp; 15=Tag-comp; 16=Tag-negbin; 17=F_ballpark; 18=initEQregime"));
        chars += c_file->writeline(QString("#like_comp  fleet  phase  value  sizefreq_method"));
        num = 0;


        for (i = 0; i < data->getNumLambdaAdjustments(); i++)
        {
            str_list = data->getLambdaAdjustment(i);
            chars += c_file->write_vector(str_list, 3);
        }
        line = QString (" -9999  1  1  1  1  # terminator");
        chars += c_file->writeline(line);
        chars += c_file->newline();

        // additional SD reporting
        temp_int = data->getAddSdReporting()->getActive();
        if (temp_int > 0)
        {
            chars += c_file->write_val(temp_int, 1, QString("(0/1/2) read specs for more stddev reporting: 0 = skip, 1 = read specs for reporting stdev for selectivity, size, and numbers, 2 = add options for M,Dyn. Bzero, SmryBio"));
            chars += c_file->write_vector(data->getAddSdReprtSelex(),     2, QString("Selectivity: (1) 0 to skip or fleet, (2) 1=len/2=age/3=combined, (3) year, (4) N selex bins; NOTE: combined reports in age bins"));
            chars += c_file->write_vector(data->getAddSdReprtGrowth(),    2, QString("Growth: (1) 0 to skip or growth pattern, (2) growth ages; NOTE: does each sex"));
            chars += c_file->write_vector(data->getAddSdReprtNumAtAge(),  2, QString("Numbers-at-age: (1) 0 or area(-1 for all), (2) year, (3) N ages;  NOTE: sums across morphs"));
            if (temp_int == 2) {
                chars += c_file->write_vector(data->getAddSdReprtNatMort(), 2, QString("Mortality: (1) 0 to skip or growth pattern, (2) N ages for mortality; NOTE: does each sex"));
                chars += c_file->write_val(data->getAddSdReporting()->getDynB0(), 1, "Dyn Bzero: 0 to skip, 1 to include, or 2 to add recr");
                chars += c_file->write_val(data->getAddSdReporting()->getSumBio(), 1, "SmryBio: 0 to skip, 1 to include");
            }
            if (!data->getAddSdReprtSelex().at(0).contains("0")) {
                chars += c_file->write_vector(data->getAddSdReprtSelexBins(), 1, QString("vector with selex std bins (-1 in first bin to self-generate)"));
            }
            else {
                chars += c_file->writeline(" # -1 # vector with selex std bins (-1 in first bin to self-generate)");
            }
            if (!data->getAddSdReprtGrowth().at(0).contains("0") &&
                    data->getReadWtAtAge() == 0) {
                chars += c_file->write_vector(data->getAddSdReprtGrwthBins(), 1, QString("vector with growth std ages picks (-1 in first bin to self-generate)"));
            }
            else {
                chars += c_file->writeline(QString(" #_-1 #_vector with growth std ages picks (-1 in first bin to self-generate)"));
            }
            if (!data->getAddSdReprtNumAtAge().at(0).contains("0")) {
                chars += c_file->write_vector(data->getAddSdReprtAtAgeBins(), 1, QString("vector with NatAge std ages (-1 in first bin to self-generate)"));
            }
            else {
                chars += c_file->writeline(" # -1 # vector with NatAge std ages (-1 in first bin to self-generate)");
            }
            if (temp_int > 1) {
                if (!data->getAddSdReprtNatMort().at(0).contains("0")) {
                    chars += c_file->write_vector(data->getAddSdReprtNatMortBins(), 1, QString("list of ages for NatM std (-1 in first bin to self-generate)"));
                }
                else {
                    chars += c_file->writeline(" # -1 # list of ages for NatM std (-1 in first bin to self-generate)");
                }
            }
        }
        else
        {
            chars += c_file->writeline(QString("0 #_(0/1/2) read specs for more stddev reporting: 0 = skip, 1 = read specs for reporting stdev for selectivity, size, and numbers, 2 = add options for M,Dyn. Bzero, SmryBio"));
            chars += c_file->writeline(QString(" # 0 2 0 0 # Selectivity: (1) fleet, (2) 1=len/2=age/3=both, (3) year, (4) N selex bins"));
            chars += c_file->writeline(QString(" # 0 0 # Growth: (1) growth pattern, (2) growth ages"));
            chars += c_file->writeline(QString(" # 0 0 0 # Numbers-at-age: (1) area(-1 for all), (2) year, (3) N ages"));
            chars += c_file->writeline(QString(" #_Cond_2 # 0 0 # Mortality: (1) 0 to skip or growth pattern, (2) N ages for mortality; NOTE: does each sex"));
            chars += c_file->writeline(QString(" #_Cond_2 # 0 # Dyn Bzero: 0 to skip, 1 to include, or 2 to add recr"));
            chars += c_file->writeline(QString(" #_Cond_2 # 0 # SmryBio: 0 to skip, 1 to include"));
            chars += c_file->writeline(QString(" # -1 # vector with selex std bins (-1 in first bin to self-generate"));
            chars += c_file->writeline(QString(" # -1 # vector with growth std ages picks (-1 in first bin to self-generate)"));
            chars += c_file->writeline(QString(" # -1 # vector with NatAge std ages (-1 in first bin to self-generate)"));
            chars += c_file->writeline(QString(" #_Cond_2 # -1 # list of ages for NatM std (-1 in first bin to self-generate)"));
        }

        chars += c_file->write_val(END_OF_DATA);
        chars += c_file->writeline();

        c_file->close();
    }
    return chars;
}

bool read33_parameterFile(ss_file *pr_file, ss_model *data)
{
    bool flag = false;
    if(pr_file->open(QIODevice::ReadOnly))
    {
        flag = true;
        pr_file->seek(0);
        pr_file->resetLineNum();
        pr_file->setOkay(true);
        pr_file->setStop(false);
        pr_file->read_comments();

        data->setALKTol(data->getALKTol());

        pr_file->close();
    }
    else
        pr_file->error("Parameter file does not exist or is not readable.");

    return flag;
}

int write33_parameterFile(ss_file *pr_file, ss_model *data)
{
    int chars = 0;

    if(pr_file->open(QIODevice::WriteOnly))
    {
        pr_file->write_comments();

        data->setALKTol(data->getALKTol());

        pr_file->close();
    }
    else
        pr_file->error("Parameter file is not writeable.");
    return chars;
}

bool read33_userDataFile (ss_file *ud_file, ss_model *data)
{
    bool flag = false;
    if(ud_file->open(QIODevice::ReadOnly))
    {
        flag = true;
        ud_file->seek(0);
        ud_file->resetLineNum();
        ud_file->setOkay(true);
        ud_file->setStop(false);
        ud_file->read_comments();

        data->setALKTol(data->getALKTol());

        ud_file->close();
    }
    else
        ud_file->error("User data file does not exist or is not readable.");

    return flag;
}

int write33_userDataFile (ss_file *ud_file, ss_model *data)
{
    int chars = 0;

    if(ud_file->open(QIODevice::WriteOnly))
    {
        ud_file->write_comments();

        data->setALKTol(data->getALKTol());

        ud_file->close();
    }
    else
        ud_file->error("User data file is not writeable.");
    return chars;
}

bool read33_profileFile (ss_file *pf_file, ss_model *data)
{
    bool okay;


    if(pf_file->open(QIODevice::ReadOnly))
    {
        pf_file->seek(0);
        pf_file->resetLineNum();
        pf_file->setOkay(true);
        pf_file->setStop(false);
        pf_file->read_comments();

        data->setALKTol(data->getALKTol());

        pf_file->close();
        okay = true;
    }
    else
    {
        pf_file->error(QString("Profile file does not exist or is not readable."));
        okay = false;
    }

    return okay;
}

int write33_profileFile (ss_file *pf_file, ss_model *data)
{
    int code = 0;

    if(pf_file->open(QIODevice::WriteOnly))
    {
        pf_file->write_comments();

        data->setALKTol(data->getALKTol());

        pf_file->close();
    }
    else
    {
        pf_file->error(QString("Profile file is not writeable."));
        code = 1;
    }

    return code;
}


void readTimeVaryParams (ss_file *infile, ss_model *data, tablemodel *paramTable, int varRead, tablemodel *varParamTable)
{
    QStringList param;
    QString header;

    int value = 0;
    int row = 0;
    if (varRead > 0)
    {
        for (int i = 0; i < paramTable->rowCount(); i++)
        {
            param = paramTable->getRowData(i);
            header = paramTable->getRowHeader(i);

            // read time varying parameters
            // blocks
            value = param.at(12).toInt();
            if (value != 0)
                row = readTimeVaryBlockParams (infile, data, value, param.at(13).toInt(), header, row, varParamTable);

            // devs
            value = param.at(8).toInt();
            if (value != 0)
                row = readTimeVaryDevParams (infile, data, value, header, row, varParamTable);

            // env link
            value = param.at(7).toInt();
            if (value != 0)
                row = readTimeVaryEnvParams (infile, data, value, header, row, varParamTable);
        }
        row = varParamTable->rowCount();
//        varParamTable->setRowCount(row);
    }
}

int readTimeVaryBlockParams(ss_file *infile, ss_model *data, int value, int fnx, QString hdr, int row, tablemodel *varParamTable)
{
    BlockPattern *bp;
    int numBlocks = 0;
    int beg;
    QStringList varParam;
//    QString varHeader(hdr);

    if (value > 0)
    {
        bp = data->getBlockPattern(value-1);
        numBlocks = bp->getNumBlocks();
        for (int i = 0; i < numBlocks; i++, row++)
        {
            beg = bp->getBlockBegin(i);
            varParam = readShortParameter(infile);
            varParamTable->setRowData(row, varParam);
            if (fnx == 0) // mult
            {
                varParamTable->setRowHeader(row, QString("%1_BLK%2mult_%3").arg(hdr, QString::number(i+1), QString::number(beg)));
            }
            else if (fnx == 1) // add
            {
                varParamTable->setRowHeader(row, QString("%1_BLK%2add_%3").arg(hdr, QString::number(i+1), QString::number(beg)));
            }
            else if (fnx == 2) // replace
            {
                varParamTable->setRowHeader(row, QString("%1_BLK%2repl_%3").arg(hdr, QString::number(i+1), QString::number(beg)));
            }
            else if (fnx == 3) // delta
            {
                varParamTable->setRowHeader(row, QString("%1_BLK%2delta_%3").arg(hdr, QString::number(i+1), QString::number(beg)));
            }
        }
    }
    else if (value == -1) // trend - offset
    {
        varParam = readShortParameter(infile);
        varParamTable->setRowData(row, varParam);
        varParamTable->setRowHeader(row++, QString("%1_TrendFinal_LogstOffset").arg(hdr));
        varParam = readShortParameter(infile);
        varParamTable->setRowData(row, varParam);
        varParamTable->setRowHeader(row++, QString("%1_TrendInfl_LogstOffset").arg(hdr));
        varParam = readShortParameter(infile);
        varParamTable->setRowData(row, varParam);
        varParamTable->setRowHeader(row++, QString("%1_TrendWidth_yrs").arg(hdr));
    }
    else if (value == -2) // trend - direct
    {
        varParam = readShortParameter(infile);
        varParamTable->setRowData(row, varParam);
        varParamTable->setRowHeader(row++, QString("%1_TrendFinal_direct").arg(hdr));
        varParam = readShortParameter(infile);
        varParamTable->setRowData(row, varParam);
        varParamTable->setRowHeader(row++, QString("%1_TrendInfl_yr").arg(hdr));
        varParam = readShortParameter(infile);
        varParamTable->setRowData(row, varParam);
        varParamTable->setRowHeader(row++, QString("%1_TrendWidth_yr").arg(hdr));
    }
    else if (value == -3) // trend - fraction
    {
        varParam = readShortParameter(infile);
        varParamTable->setRowData(row, varParam);
        varParamTable->setRowHeader(row++, QString("%1_TrendFinal_frac").arg(hdr));
        varParam = readShortParameter(infile);
        varParamTable->setRowData(row, varParam);
        varParamTable->setRowHeader(row++, QString("%1_TrendInfl_frac").arg(hdr));
        varParam = readShortParameter(infile);
        varParamTable->setRowData(row, varParam);
        varParamTable->setRowHeader(row++, QString("%1_TrendWidth_yr").arg(hdr));
    }
    else if (value <= -4) // trend - seasonal
    {
        int seas = data->get_num_seasons();
        for (int i = 0; i < seas; i++, row++)
        {
            varParam = readShortParameter(infile);
            varParamTable->setRowData(row, varParam);
            varParamTable->setRowHeader(row, QString("%1_TrendFinal_seas%2").arg(hdr, QString::number(i+1)));
        }
    }
    return (row);
}

int readTimeVaryDevParams(ss_file *infile, ss_model *data, int value, QString hdr, int row, tablemodel *varParamTable)
{
    QStringList varParam;
    QString varHeader(hdr);
    Q_UNUSED(data)
    Q_UNUSED(value)
//    switch (value)
    {
//    case 1:
//    case 2:
        varParam = readShortParameter(infile);
        varParamTable->setRowData(row, varParam);
        varParamTable->setRowHeader(row, QString("%1_dev_se").arg(varHeader));
        row++;
        varParam = readShortParameter(infile);
        varParamTable->setRowData(row, varParam);
        varParamTable->setRowHeader(row, QString("%1_dev_autocorr").arg(varHeader));
//        break;
    }
    return (row + 1);
}

int readTimeVaryEnvParams(ss_file *infile, ss_model *data, int value, QString hdr, int row, tablemodel *varParamTable)
{
    int fnx = value / 100;
    int var = value - (fnx * 100);
    QStringList varParam;
    QString varHeader(hdr);
    Q_UNUSED(data)

    if (var > 0)
    {
        varParam = readShortParameter(infile);
        varParamTable->setRowData(row, varParam);
        switch (fnx)
        {
        default:
        case 0:
            varHeader.append(QString("_ENV_none"));
            break;
        case 1:
            varHeader.append(QString("_ENV_add"));
            break;
        case 2:
            varHeader.append(QString("_ENV_mult"));
            break;
        case 3:
            varHeader.append(QString("_ENV_offset"));
            break;
        case 4:
            varHeader.append(QString("_ENV_lgst_slope"));
            break;
        }
        varParamTable->setRowHeader(row, varHeader);
    }
    return (row + 1);
}

int writeTimeVaryParams(ss_file *outfile, ss_model *data, tablemodel *table, QStringList parmlist)
{
    QStringList p_list;
    QString rheader;
    int index = 0;
//    int last_index = table->rowCount();
    int chars = 0;
    int blk = parmlist.at(12).toInt();

    // write parameters for block or trend
    if (blk > 0)
    {
        // write block parameters
        int n_blks = data->getBlockPattern(blk-1)->getNumBlocks();
        for (int i = 0; i < n_blks; i++)
        {
            p_list = table->getRowData(index);
            rheader = table->getRowHeader(index++);
            chars += outfile->write_vector(p_list, 2, rheader);
        }
    }
    //  or write trend parameters
    else if (blk < 0)
    {
        p_list = table->getRowData(index);
        rheader = table->getRowHeader(index++);
        chars += outfile->write_vector(p_list, 2, rheader);
        p_list = table->getRowData(index);
        rheader = table->getRowHeader(index++);
        chars += outfile->write_vector(p_list, 2, rheader);
        p_list = table->getRowData(index);
        rheader = table->getRowHeader(index++);
        chars += outfile->write_vector(p_list, 2, rheader);
    }

    // write parameter for env
    if (parmlist.at(7).toInt() != 0)
    {
        p_list = table->getRowData(index);
        rheader = table->getRowHeader(index++);
        chars += outfile->write_vector(p_list, 2, rheader);
    }

    // write parameters for dev
    if (parmlist.at(8).toInt() != 0)
    {
        p_list = table->getRowData(index);
        rheader = table->getRowHeader(index++);
        chars += outfile->write_vector(p_list, 2, rheader);
        p_list = table->getRowData(index);
        rheader = table->getRowHeader(index++);
        chars += outfile->write_vector(p_list, 2, rheader);
    }
    return chars;
}

bool negateParameterPhase(QStringList &datalist)
{
    bool okay = true;
    if (datalist.count() > 5) {
        float phase = datalist.at(6).toFloat();
        if (phase > 0)
            datalist[6] = QString::number(-phase);
    }
    return okay;
}
