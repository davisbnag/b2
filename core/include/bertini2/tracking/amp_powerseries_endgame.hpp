//This file is part of Bertini 2.0.
//
//powerseries_endgame.hpp is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//powerseries_endgame.hpp is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with predict.hpp.  If not, see <http://www.gnu.org/licenses/>.
//

//  powerseries_endgame.hpp
//
//  copyright 2015
//  Tim Hodges
//  Colorado State University
//  Department of Mathematics
//  Fall 2015



#ifndef BERTINI_TRACKING_POWERSERIES_HPP
#define BERTINI_TRACKING_POWERSERIES_HPP

/**
\file powerseries_endgame.hpp

\brief Contains the power series endgame type, and necessary functions.
*/
#include <deque>
#include <boost/multiprecision/gmp.hpp>
#include <iostream>
#include "tracking/amp_endgame.hpp"
#include <cstdio>
#include <deque>

namespace bertini{ 

	namespace tracking {

		namespace endgame {

		/** 
		\class PowerSeriesEndgame

		\brief class used to finish tracking paths during Homotopy Continuation
		
		
		## Explanation
		
		The bertini::PowerSeriesEndgame class enables us to finish tracking on possibly singular paths on an arbitrary square homotopy.  

		The intended usage is to:

		1. Create a system, tracker, and instantiate some settings.
		2. Using the tracker created track to the engame boundary. 
		3. Create a PowerSeriesEndgame, associating it to the system you are going to solve or track on.
		4. For each path being tracked send the PowerSeriesEndgame the time value and other variable values at that time. 
		5. The PowerSeriesEndgame, if successful, will store the target systems solution at t = 0.

		## Example Usage
		
		Below we demonstrate a basic usage of the PowerSeriesEndgame class to find the singularity at t = 0. 

		The pattern is as described above: create an instance of the class, feeding it the system to be used, and the endgame boundary time and other variable values at the endgame boundary. 

		\code{.cpp}
		mpfr_float::default_precision(30); // set initial precision.  This is not strictly necessary.
		using namespace bertini::tracking;

		// 1. Create the system
		System sys;
		Var x = std::make_shared<Variable>("x"), t = std::make_shared<Variable>("t"), y = std::make_shared<Variable>("y");
		VariableGroup vars{x,y};
		sys.AddVariableGroup(vars); 
		sys.AddPathVariable(t);
		// Define homotopy system
		sys.AddFunction((pow(x-1,3))*(1-t) + (pow(x,3) + 1)*t);
		sys.AddFunction((pow(y-1,2))*(1-t) + (pow(y,2) + 1)*t);

		//2. Setup a tracker. 
		bertini::tracking::AMPTracker tracker(sys);
	
		bertini::tracking::config::Stepping stepping_preferences;
		bertini::tracking::config::Newton newton_preferences;

		tracker.Setup(bertini::tracking::config::Predictor::Euler,
                mpfr_float("1e-5"),
                mpfr_float("1e5"),
                stepping_preferences,
                newton_preferences);
	
		tracker.AMPSetup(AMP);

		// We make the assumption that we have tracked to t = 0.1 (value at t = 0.1 known from Bertini 1.5)
		mpfr current_time(1);
		Vec<mpfr> current_space(2);
		current_time = mpfr(".1");
		current_space <<  mpfr("5.000000000000001e-01", "9.084258952712920e-17") ,mpfr("9.000000000000001e-01","4.358898943540673e-01");

		//  3. (Optional) configure settings for specific endgame. 
		bertini::tracking::config::Endgame endgame_settings;
		bertini::tracking::config::PowerSeries power_series_settings;
		bertini::tracking::config::Security endgame_security_settings;

		//4. Create the PowerSeriesEngame object, by sending in the tracker and any other settings. Notice the endgame is templated by the tracker. 
		bertini::tracking::endgame::PowerSeriesEndgame<bertini::tracking::AMPTracker> My_Endgame(tracker,power_series_settings,endgame_settings,endgame_security_settings);

		//Calling the PSEG member function actually runs the endgame. 
		My_Endgame.PSEG(current_time,current_space);

		//Access solution at t = 0, using the .get_final_approximation_at_origin() member function. 
		auto Answer  = My_Endgame.get_final_approximation_at_origin();
		\endcode
		
		If this documentation is insufficient, please contact the authors with suggestions, or get involved!  Pull requests welcomed.
		
		## Testing

		Test suite driving this class: endgames_test.

		File: test/endgames/powerseries_class_test.cpp

		Functionality tested: All member functions of the PowerSeriesEndgame have been tested in variable precision. There is also, test running different systems to find singular solutions at t = 0.
		*/
		
			template<typename TrackerType> 
			class PowerSeriesEndgame : public AMPEndgame<TrackerType>
			{
			private:

				/**
				\brief Settings that are specific to the Power series endgame. 
				*/	
				config::PowerSeries power_series_settings_; 

				/**
				\brief A deque holding the time values for different space values used in the Power series endgame. 
				*/	
				mutable std::tuple< TimeCont<dbl>, TimeCont<mpfr> > times_;
		
				/**
				\brief A deque holding the space values used in the Power series endgame. 
				*/			
				mutable std::tuple< SampCont<dbl>, SampCont<mpfr> > samples_;

				/**
				\brief State variable representing a computed upper bound on the cycle number.
				*/
				mutable unsigned upper_bound_on_cycle_number_;



				void ChangeTemporariesPrecisionImpl(unsigned new_precision) const override
				{ }

				void MultipleToMultipleImpl(unsigned new_precision) const override
				{
					auto& times = std::get<TimeCont<mpfr> >(times_);
					for (auto& t : times)
						t.precision(new_precision);

					auto& samples = std::get< SampCont<mpfr> >(samples_);
					for (auto& s : samples)
						for (unsigned ii=0; ii<s.size(); ++ii)
							s(ii).precision(new_precision);
				}

				void DoubleToMultipleImpl(unsigned new_precision) const override
				{
					auto& times_m = std::get<TimeCont<mpfr> >(times_);
					auto& times_d = std::get<TimeCont<dbl> >(times_);

					for (unsigned ii=0; ii<times_m.size(); ++ii)
					{
						times_m[ii].precision(new_precision);
						times_m[ii] = mpfr(times_d[ii]);
					}


					auto& samples_m = std::get< SampCont<mpfr> >(samples_);
					auto& samples_d = std::get< SampCont<dbl > >(samples_);

					for (unsigned ii=0; ii<times_m.size(); ++ii)
					{
						auto& s = samples_m[ii];
						auto& sd = samples_d[ii];
						for (unsigned jj=0; jj<s.size(); ++jj)
						{
							s(jj).precision(new_precision);
							s(jj) = mpfr(sd(jj));
						}
					}
				}

				void MultipleToDoubleImpl() const override
				{
					auto& times_m = std::get<TimeCont<mpfr> >(times_);
					auto& times_d = std::get<TimeCont<dbl> >(times_);

					for (unsigned ii=0; ii<times_m.size(); ++ii)
					{
						times_d[ii] = dbl(times_m[ii]);
					}


					auto& samples_m = std::get< SampCont<mpfr> >(samples_);
					auto& samples_d = std::get< SampCont<dbl > >(samples_);

					for (unsigned ii=0; ii<times_m.size(); ++ii)
					{
						auto& s = samples_m[ii];
						auto& sd = samples_d[ii];
						for (unsigned jj=0; jj<s.size(); ++jj)
						{
							sd(jj) = dbl(s(jj));
						}
					}
				}

			public:

				auto UpperBoundOnCycleNumber() const { return upper_bound_on_cycle_number_;}

				const config::PowerSeries& PowerSeriesSettings() const
				{
					return power_series_settings_;
				}

				/**
				\brief Function that clears all samples and times from data members for the Power Series endgame
				*/	
				template<typename CT>
				void ClearTimesAndSamples()
				{
					std::get<TimeCont<CT> >(times_).clear(); 
					std::get<SampCont<CT> >(samples_).clear();
				}

				/**
				\brief Function to set the times used for the Power Series endgame.
				*/	
				template<typename CT>
				void SetTimes(TimeCont<CT> times_to_set) { std::get<TimeCont<CT> >(times_) = times_to_set;}

				/**
				\brief Function to get the times used for the Power Series endgame.
				*/	
				template<typename CT>
				auto GetTimes() const {return std::get<TimeCont<CT> >(times_);}

				/**
				\brief Function to set the space values used for the Power Series endgame.
				*/	
				template<typename CT>
				void SetSamples(SampCont<CT> samples_to_set) { std::get<SampCont<CT> >(samples_) = samples_to_set;}

				/**
				\brief Function to get the space values used for the Power Series endgame.
				*/	
				template<typename CT>
				auto GetSamples() const {return std::get<SampCont<CT> >(samples_);}
	

				/**
				\brief Setter for the specific settings in tracking_conifg.hpp under PowerSeries.
				*/	
				void SetPowerSeriesSettings(config::PowerSeries new_power_series_settings){power_series_settings_ = new_power_series_settings;}


				explicit PowerSeriesEndgame(TrackerType const& tr, 
				                            const std::tuple< config::PowerSeries const&, 
				                            					const config::Endgame&, 
				                            					const config::Security&, 
				                            					const config::Tolerances& >& settings )
			      : AMPEndgame<TrackerType>(tr, std::get<1>(settings), std::get<2>(settings), std::get<3>(settings) ), 
			          power_series_settings_( std::get<0>(settings) )
			   	{}

			    template< typename... Ts >
   				PowerSeriesEndgame(TrackerType const& tr, const Ts&... ts ) : PowerSeriesEndgame(tr, Unpermute<config::PowerSeries, config::Endgame, config::Security, config::Tolerances >( ts... ) ) 
   				{}


				~PowerSeriesEndgame() {};


			/*
			Input: No input, all data needed is class data members.

			Output: No output, the upper bound calculated is stored in the power series settings defined in tracking_config.hpp

			Details:
					Using the formula for the cycle test outline in the Bertini Book pg. 53, we can compute an upper bound
					on the cycle number. This upper bound is used for an exhaustive search in ComputeCycleNumber for the actual cycle number. 
			*/

			/**
			\brief Function that computes an upper bound on the cycle number. Consult page 53 of \cite Bates .
			*/
			template<typename CT>
			void ComputeBoundOnCycleNumber()
			{ 
				using RT = typename Eigen::NumTraits<CT>::Real;

				const auto& samples = std::get<SampCont<CT> >(samples_);

				const Vec<CT> & sample0 = samples[0];
				const Vec<CT> & sample1 = samples[1];
				const Vec<CT> & sample2 = samples[2];

				Vec<CT> rand_vector = Vec<CT>::Random(sample0.size()); //should be a row vector for ease in multiplying.
				

				// //DO NOT USE Eigen .dot() it will do conjugate transpose which is not what we want.
				// //Also, the .transpose*rand_vector returns an expression template that we do .norm of since abs is not available for that expression type. 
				RT estimate = abs(log(abs((((sample2 - sample1).transpose()*rand_vector).norm())/(((sample1 - sample0).transpose()*rand_vector).norm()))));
				estimate = abs(log(this->EndgameSettings().sample_factor))/estimate;
				if (estimate < 1)
				{
				  	upper_bound_on_cycle_number_ = 1;
				}
				else
				{
					//casting issues between auto and unsigned integer. TODO: Try to stream line this.
					unsigned int upper_bound;
					RT upper_bound_before_casting = round(floor(estimate + RT(.5))*power_series_settings_.cycle_number_amplification);
					upper_bound = unsigned (upper_bound_before_casting);
					upper_bound_on_cycle_number_ = max(upper_bound,power_series_settings_.max_cycle_number);
				}
				// Make sure to use Eigen and transpose to use Linear algebra. DO NOT USE Eigen .dot() it will do conjugate transpose which is not what we want. 
				// need to have cycle_number_amplification, this is the 5 used for 5 * estimate

			}//end ComputeBoundOnCycleNumber


			/*
			Input:	 
			    	current_time is the time value that we wish to interpolate at and compare against the hermite value for each possible cycle number.
			    	x_current_time is the corresponding space value at current_time.
			    	upper_bound_on_cycle_number is the largest possible cycle number we will use to compute dx_ds and s = t^(1/c).

					samples are space values that correspond to the time values in times. 
					derivatives are the dx_dt or dx_ds values at the (time,sample) values.
					num_sample_points is the size of samples, times, derivatives, this is also the amount of information used in Hermite Interpolation.

			Output: 
					A set of derivatives for the samples and times given. The cycle number is stored in the endgame settings defined in tracking_config.hpp

			Details: 
					This is done by an exhaustive search from 1 to upper_bound_on_cycle_number. There is a conversion to the s-space from t-space in this function. 

			*/

			/**
			\brief This function computes the cycle number using an exhaustive search up the upper bound computed by the above function BoundOnCyleNumber. 

			As a by-product the derivatives at each of the samples is returned for further use. 

			\param[out] time is the last time inside of the times_ deque.
			\param x_at_time is the last sample inside of the samples_ deque. 

			\tparam CT The complex number type.
			*/		
			template<typename CT>
			SampCont<CT> ComputeCycleNumber()
			{
				using RT = typename Eigen::NumTraits<CT>::Real;

				const auto& samples = std::get<SampCont<CT> >(samples_);
	 			const auto& times   = std::get<TimeCont<CT> >(times_);

	 			assert((samples.size() == times.size()) && "must have same number of times and samples");
				assert((samples.size() >= 3) && "must have at least 3 samples");

	 			auto num_samples = samples.size();
				//Compute dx_dt for each sample.
				SampCont<CT> derivatives(num_samples);
				for(unsigned ii = 0; ii < num_samples; ++ii)
				{	
					BOOST_LOG_TRIVIAL(severity_level::trace) << mpfr_float::default_precision() << " " << this->GetSystem().precision() << " " << Precision(samples[ii](0)) << std::endl;
					// uses LU look at Eigen documentation on inverse in Eigen/LU.
				 	derivatives[ii] = -(this->GetSystem().Jacobian(samples[ii],times[ii]).inverse())*(this->GetSystem().TimeDerivative(samples[ii],times[ii]));
				}

				//Compute upper bound for cycle number.
				ComputeBoundOnCycleNumber<CT>();

				const Vec<CT>& most_recent_sample = samples.back();
				const CT& most_recent_time        = times.back();

				//Compute Cycle Number
				 //num_sample_points - 1 because we are using the most current sample to do an exhaustive search for the best cycle number. 
				auto num_used_points = num_samples-1;

				auto min_found_difference = Eigen::NumTraits<RT>::highest();

				SampCont<CT> temp_samples = samples; // take a copy
				temp_samples.pop_back(); // remove the last item from it.  
				TimeCont<CT> s_times(num_used_points);
				SampCont<CT> s_derivatives(num_used_points);

				for(unsigned int candidate = 1; candidate <= upper_bound_on_cycle_number_; ++candidate)
				{			
					for(unsigned int ii=0; ii<num_used_points; ++ii)// using the last sample to predict to. 
					{ 
						s_times[ii] = pow(times[ii],RT(1)/RT(candidate));
						s_derivatives[ii] = derivatives[ii] * (candidate * pow(times[ii], static_cast<RT>(candidate-1)/candidate));
					}

					auto curr_diff = (HermiteInterpolateAndSolve(
					                      pow(most_recent_time,static_cast<RT>(1)/candidate),
					                      num_used_points,s_times,temp_samples,s_derivatives) 
					                 - most_recent_sample).norm();

					if(curr_diff < min_found_difference)
					{
						min_found_difference = curr_diff;
						this->cycle_number_ = candidate;
					}

				}// end cc loop over cycle number possibilities

				return derivatives;
			}//end ComputeCycleNumber


			/*
			Input: time_t0 is the time value that we wish to interpolate at.


			Output: A new sample point that was found by interpolation to time_t0.

			Details: This function handles computing an approximation at the origin. First all samples are brought to the same precision. After this we refine all samples 
				to final tolerance to aid in better approximations. 
				We compute the cycle number best for the approximation, and convert derivatives and times to the s-plane where s = t^(1/c).
				We use the converted times and derivatives along with the samples to do a Hermite interpolation which is found in base_endgame.hpp.

			*/

			/**
			\brief This function computes an approximation of the space value at the time time_t0. 

			This function will compute the cycle number and then dialate the time and derivatives accordingly. After dialation this function will 
			call a Hermite interpolater to interpolate to the time that we were passed in. 

			\param[out] t0 is the time value corresponding to the space value we are trying to approximate.

			\tparam CT The complex number type.
			*/
			template<typename CT>
			SuccessCode ComputeApproximationOfXAtT0(Vec<CT>& result, const CT & t0)
			{	
				using RT = typename Eigen::NumTraits<CT>::Real;

				auto& samples = std::get<SampCont<CT> >(samples_);
	 			auto& times   = std::get<TimeCont<CT> >(times_);

	 			assert(samples.size()==times.size() && "must have same number of samples in times and spaces");

				EnsureAtUniformPrecision(samples, times);

				for (unsigned ii = 0; ii < samples.size(); ++ii)
				{	
					auto refine_success = this->RefineSample(samples[ii],samples[ii],times[ii]);
					if (refine_success!=SuccessCode::Success)
						return refine_success;
				}

				auto derivatives = ComputeCycleNumber<CT>();
				auto c = this->CycleNumber();

				// //Conversion to S-plane.
				TimeCont<CT> s_times;
				SampCont<CT> s_derivatives;

				for(unsigned ii = 0; ii < samples.size(); ++ii){
					
					if (c==0)
						throw std::runtime_error("cycle number is 0 while computing approximation of root at target time");

					s_derivatives.push_back(derivatives[ii]*( c*pow(times[ii],RT(c-1)/c )));
					s_times.push_back(pow(times[ii],RT(1)/c));
				}

				result = bertini::tracking::endgame::HermiteInterpolateAndSolve(t0, this->EndgameSettings().num_sample_points, s_times, samples, s_derivatives);
				return SuccessCode::Success;
			}//end ComputeApproximationOfXAtT0


			/*
			Input: start_time is the endgame boundary default set to .1 and start_point is the space value we are given at start_time


			Output: SuccessCode if we were successful or if we have encountered an error. 

			Details: 
					Using successive hermite interpolations with a geometric progression of time values, we attempt to find the value 
					of the homotopy at time t = 0.
			*/

			/**
			\brief This function runs the Power Series endgame. 

			Tracking forward with the number of sample points, this function will make approximations using Hermite interpolation. This process will continue until two consecutive
			approximations are withing final tolerance of each other. 

			\param[out] start_time The time value at which we start the endgame. 
			\param endgame_sample The current space point at start_time.

			\tparam CT The complex number type.
			*/		
			template<typename CT>
			SuccessCode PSEG(const CT & start_time, const Vec<CT> & start_point)
			{
				if (start_point.size()!=this->GetSystem().NumVariables())
				{
					std::stringstream err_msg;
					err_msg << "number of variables in start point for PSEG, " << start_point.size() << ", must match the number of variables in the system, " << this->GetSystem().NumVariables();
					throw std::runtime_error(err_msg.str());
				}

				BOOST_LOG_TRIVIAL(severity_level::trace) << "\n\nPSEG(), default precision: " << mpfr_float::default_precision() << "\n\n";
				BOOST_LOG_TRIVIAL(severity_level::trace) << "start point precision: " << Precision(start_point(0)) << "\n\n";

				mpfr_float::default_precision(Precision(start_point(0)));

				using RT = typename Eigen::NumTraits<CT>::Real;
				//Set up for the endgame.
	 			ClearTimesAndSamples<CT>();

	 			auto& samples = std::get<SampCont<CT> >(samples_);
	 			auto& times   = std::get<TimeCont<CT> >(times_);

			 	RT approx_error(1);  //setting up the error of successive approximations. 
			 	
			 	CT origin(0);

				auto initial_sample_success = this->ComputeInitialSamples(start_time, start_point, times, samples);
				if (initial_sample_success!=SuccessCode::Success)
				{
					BOOST_LOG_TRIVIAL(severity_level::trace) << "initial sample gathering failed, code " << int(initial_sample_success) << std::endl;
					return initial_sample_success;
				}

			 	Vec<CT> prev_approx;
			 	auto extrapolation_code = ComputeApproximationOfXAtT0(prev_approx, origin);
			 	if (extrapolation_code != SuccessCode::Success)
			 		return extrapolation_code;


			 	RT norm_of_dehom_of_prev_approx;
			 	if(this->SecuritySettings().level <= 0)
			 	 	norm_of_dehom_of_prev_approx = this->GetSystem().DehomogenizePoint(prev_approx).norm();

			 	

			 	CT current_time = times.back(); 

			  	Vec<CT> next_sample;
			  	Vec<CT> latest_approx;
			  	Vec<CT>& final_approx = std::get<Vec<CT> >(this->final_approximation_at_origin_);
			    RT norm_of_dehom_of_latest_approx;


				while (approx_error > this->Tolerances().final_tolerance)
				{
					final_approx = prev_approx;
			  		 auto next_time = times.back() * this->EndgameSettings().sample_factor; //setting up next time value.

			  		if (next_time.abs() < this->EndgameSettings().min_track_time)
			  		{
			  			BOOST_LOG_TRIVIAL(severity_level::trace) << "Current time norm is less than min track time." << '\n';

			  			final_approx = prev_approx;
			  			return SuccessCode::MinTrackTimeReached;
			  		}

			  		BOOST_LOG_TRIVIAL(severity_level::trace) << "tracking to t = " << next_time << ", default precision: " << mpfr_float::default_precision() << "\n";
					SuccessCode tracking_success = this->GetTracker().TrackPath(next_sample,current_time,next_time,samples.back());
					if (tracking_success != SuccessCode::Success)
						return tracking_success;


					//push most current time into deque, and lose the least recent time.
			 		current_time = next_time;
			 		times.push_back(next_time);
			 		times.pop_front(); 


			 		samples.push_back(next_sample);
			 		samples.pop_front();

			 		assert(samples.size()==times.size() && "samples and times must be of same size here");

			 		auto approx_code = ComputeApproximationOfXAtT0(latest_approx, origin);
			 		if (approx_code!=SuccessCode::Success)
			 		{
			 			BOOST_LOG_TRIVIAL(severity_level::trace) << "failed to compute the approximation at " << origin << "\n\n";
			 			return approx_code;
			 		}

			 		BOOST_LOG_TRIVIAL(severity_level::trace) << "latest approximation:\n" << latest_approx << '\n';

			 		if(this->SecuritySettings().level <= 0)
			 		{
			 			norm_of_dehom_of_latest_approx = this->GetSystem().DehomogenizePoint(latest_approx).norm();

				 		if(norm_of_dehom_of_latest_approx > this->SecuritySettings().max_norm && norm_of_dehom_of_prev_approx > this->SecuritySettings().max_norm)
			 				return SuccessCode::SecurityMaxNormReached;
			 		}


			 		approx_error = (latest_approx - prev_approx).norm();

			 		BOOST_LOG_TRIVIAL(severity_level::trace) << "corresponding error:\n" << approx_error << '\n';

			 		if(approx_error < this->Tolerances().final_tolerance)
			 		{//success!
			 			final_approx = latest_approx;
			 			return SuccessCode::Success;
			 		}

			 		prev_approx = latest_approx;

			 		if(this->SecuritySettings().level <= 0)
					    norm_of_dehom_of_prev_approx = norm_of_dehom_of_latest_approx;
				} //end while	
				// in case if we get out of the for loop without setting. 
				final_approx = latest_approx;
				return SuccessCode::Success;

			} //end PSEG

		}; // end powerseries class




		}//namespace endgame

	}//namespace tracking

} // namespace bertini
#endif
