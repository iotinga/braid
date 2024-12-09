package io.tinga.braid.etlgadget;

import java.util.Arrays;
import java.util.List;
import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.TypeLiteral;

import io.tinga.belt.AbstractGadget;
import io.tinga.belt.config.ConfigurationProvider;
import io.tinga.belt.input.GadgetCommandOption;
import io.tinga.belt.output.GadgetInMemoryPlainTextSink;
import io.tinga.belt.output.GadgetSink;
import io.tinga.braid.etlgadget.dbloader.DbBasicConnectionManager;
import io.tinga.braid.etlgadget.dbloader.DbConfig;
import io.tinga.braid.etlgadget.dbloader.DbConfigImpl;
import io.tinga.braid.etlgadget.dbloader.DbConnectionManager;
import io.tinga.braid.etlgadget.dbloader.DbLoader;
import io.tinga.braid.etlgadget.etl.BasicEtlEventHandler;
import io.tinga.braid.etlgadget.etl.BasicExtractor;
import io.tinga.braid.etlgadget.etl.BasicTransformer;
import io.tinga.braid.etlgadget.etl.EtlEventHandler;
import io.tinga.braid.etlgadget.etl.Extractor;
import io.tinga.braid.etlgadget.etl.Loader;
import io.tinga.braid.etlgadget.etl.ShadowEvent;
import io.tinga.braid.etlgadget.etl.ShadowRecord;
import io.tinga.braid.etlgadget.etl.Transformer;
import it.netgrid.bauer.TopicFactory;

public class EtlGadget extends AbstractGadget<EtlGadgetCommandExecutor, EtlGadgetCommand> {
    Logger log = LoggerFactory.getLogger(EtlGadget.class);

    @Override
    protected void configure() {
        bind(GadgetSink.class).to(GadgetInMemoryPlainTextSink.class);
        bind(EtlEventHandler.class).to(BasicEtlEventHandler.class);
        bind(DbConnectionManager.class).to(DbBasicConnectionManager.class).in(Singleton.class);
        bind(new TypeLiteral<Extractor<ShadowEvent, ShadowEvent>>() {
        }).to(BasicExtractor.class);
        bind(new TypeLiteral<Transformer<ShadowEvent, List<ShadowRecord>>>() {
        }).to(BasicTransformer.class);
        bind(new TypeLiteral<Loader<List<ShadowRecord>>>() {
        }).to(DbLoader.class);
    }

    @Provides
    @Singleton
    public DbConfig buildDbConfig(ConfigurationProvider provider) {
        return provider.config("BRAID_DB", DbConfigImpl.class);
    }

    @Provides
    @Singleton
    public GadgetConfig buildGadgetConfig(ConfigurationProvider provider) {
        return provider.config("ETL", GadgetConfigImpl.class);
    }

    @Override
    public String name() {
        return "ETL";
    }

    @Override
    public Class<EtlGadgetCommand> commandClass() {
        return EtlGadgetCommand.class;
    }

    @Override
    public Class<EtlGadgetCommandExecutor> executorClass() {
        return EtlGadgetCommandExecutor.class;
    }

    @Override
    public List<GadgetCommandOption> commandOptions() {
        return Arrays.asList(EtlGadgetCommandOption.values());
    }

    @Override
    public Module[] buildExecutorModules(Properties properties, EtlGadgetCommand command) {
        log.debug("Building executor modules with properties {}", properties);
        Module[] retval = { TopicFactory.getAsModule(properties), new EtlGadgetExecutorModule() };
        return retval;
    }

}
