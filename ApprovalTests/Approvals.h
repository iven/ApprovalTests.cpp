#pragma once
#include <string>
#include <functional>
#include <exception>
#include <utility>

#include "ApprovalTests/ApprovalTestsVersion.h"
#include "ApprovalTests/core/FileApprover.h"
#include "reporters/DefaultReporter.h"
#include "reporters/DefaultReporterDisposer.h"
#include "ApprovalTests/core/Reporter.h"
#include "namers/ApprovalTestNamer.h"
#include "writers/ExistingFile.h"
#include "namers/ExistingFileNamer.h"
#include "namers/SubdirectoryDisposer.h"
#include "namers/DefaultNamerDisposer.h"
#include "scrubbers/Scrubbers.h"
#include "core/Options.h"
#include "utilities/MoreHelpMessages.h"
#include "utilities/StringMaker.h"

namespace ApprovalTests
{

    // TCompileTimeOptions must have a type ToStringConverter, which must have a method toString()
    template <typename TCompileTimeOptions> class TApprovals
    {
    private:
        TApprovals() = default;

        ~TApprovals() = default;

    public:
        static std::shared_ptr<ApprovalNamer> getDefaultNamer()
        {
            return DefaultNamerFactory::getDefaultNamer()();
        }

        template <typename T>
        using IsNotDerivedFromWriter =
            typename std::enable_if<!std::is_base_of<ApprovalWriter, T>::value,
                                    int>::type;

        /**@name Verifying single objects

         See \userguide{TestingSingleObjects,Testing Single Objects}
         */
        ///@{
        static void verify(const std::string& contents,
                           const Options& options = Options())
        {
            StringWriter writer(options.scrub(contents),
                                options.fileOptions().getFileExtension());
            FileApprover::verify(*getDefaultNamer(), writer, options.getReporter());
        }

        /// Note that this overload ignores any scrubber in options
        static void verify(const ApprovalWriter& writer,
                           const Options& options = Options())
        {
            FileApprover::verify(*getDefaultNamer(), writer, options.getReporter());
        }

        template <typename T, typename = IsNotDerivedFromWriter<T>>
        static void verify(const T& contents, const Options& options = Options())
        {
            verify(TCompileTimeOptions::ToStringConverter::toString(contents), options);
        }

        template <typename T,
                  typename Function,
                  typename = Detail::EnableIfNotOptions<Function>>
        static void
        verify(const T& contents, Function converter, const Options& options = Options())
        {
            std::stringstream s;
            converter(contents, s);
            verify(s.str(), options);
        }
        ///@}

        static void
        verifyExceptionMessage(const std::function<void(void)>& functionThatThrows,
                               const Options& options = Options())
        {
            std::string message = "*** no exception thrown ***";
            try
            {
                functionThatThrows();
            }
            catch (const std::exception& e)
            {
                message = e.what();
            }
            verify(message, options);
        }

        /**@name Verifying containers of objects

         See \userguide{TestingContainers,Testing Containers}
         */
        ///@{
        template <typename Iterator>
        static void verifyAll(
            const std::string& header,
            const Iterator& start,
            const Iterator& finish,
            std::function<void(typename Iterator::value_type, std::ostream&)> converter,
            const Options& options = Options())
        {
            std::stringstream s;
            if (!header.empty())
            {
                s << header << "\n\n\n";
            }
            for (auto it = start; it != finish; ++it)
            {
                converter(*it, s);
                s << '\n';
            }
            verify(s.str(), options);
        }

        template <typename Container>
        static void verifyAll(
            const std::string& header,
            const Container& list,
            std::function<void(typename Container::value_type, std::ostream&)> converter,
            const Options& options = Options())
        {
            verifyAll<typename Container::const_iterator>(
                header, list.begin(), list.end(), converter, options);
        }

        template <typename T>
        static void verifyAll(const std::string& header,
                              const std::vector<T>& list,
                              const Options& options = Options())
        {
            int i = 0;
            verifyAll<std::vector<T>>(
                header,
                list,
                [&](T e, std::ostream& s) {
                    s << "[" << i++
                      << "] = " << TCompileTimeOptions::ToStringConverter::toString(e);
                },
                options);
        }

        template <typename T>
        static void verifyAll(const std::vector<T>& list,
                              const Options& options = Options())
        {
            verifyAll<T>("", list, options);
        }
        ///@}

        static void verifyExistingFile(const std::string& filePath,
                                       const Options& options = Options())
        {
            ExistingFile writer(filePath, options);
            FileApprover::verify(writer.getNamer(), writer, options.getReporter());
        }

        /**@name Customising Approval Tests

         These static methods customise various aspects
         of Approval Tests behaviour.
         */
        ///@{

        /// See \userguide{Configuration,using-sub-directories-for-approved-files,Using sub-directories for approved files}
        static SubdirectoryDisposer
        useApprovalsSubdirectory(const std::string& subdirectory = "approval_tests")
        {
            return SubdirectoryDisposer(subdirectory);
        }

        /// See \userguide{Reporters,registering-a-default-reporter,Registering a default reporter}
        static DefaultReporterDisposer
        useAsDefaultReporter(const std::shared_ptr<Reporter>& reporter)
        {
            return DefaultReporterDisposer(reporter);
        }

        /// See \userguide{Reporters,front-loaded-reporters,Front Loaded Reporters}
        static FrontLoadedReporterDisposer
        useAsFrontLoadedReporter(const std::shared_ptr<Reporter>& reporter)
        {
            return FrontLoadedReporterDisposer(reporter);
        }

        /// See \userguide{Namers,registering-a-custom-namer,Registering a Custom Namer}
        static DefaultNamerDisposer useAsDefaultNamer(NamerCreator namerCreator)
        {
            return DefaultNamerDisposer(std::move(namerCreator));
        }
        ///@}
    };

#ifndef APPROVAL_TESTS_DEFAULT_STREAM_CONVERTER
// begin-snippet: customising_to_string_default_converter
#define APPROVAL_TESTS_DEFAULT_STREAM_CONVERTER StringMaker
// end-snippet
#endif

    // Warning: Do not use CompileTimeOptions directly.
    // This interface is subject to change, as future
    // compile-time options are added.
    template <typename TToString> struct CompileTimeOptions
    {
        using ToStringConverter = TToString;
        // more template types may be added to CompileTimeOptions in future, if we add
        // more flexibility that requires compile-time configuration.
    };

    // Template parameter TToString must have a method toString()
    // This interface will not change, as future compile-time options are added.
    template <typename TToString>
    struct ToStringCompileTimeOptions : CompileTimeOptions<TToString>
    {
    };

    using Approvals =
        TApprovals<ToStringCompileTimeOptions<APPROVAL_TESTS_DEFAULT_STREAM_CONVERTER>>;
}
